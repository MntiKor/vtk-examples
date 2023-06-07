#!/usr/bin/env python3

import shutil
import tempfile
from pathlib import Path

import pandas as pd
# noinspection PyUnresolvedReferences
import vtkmodules.vtkInteractionStyle
# noinspection PyUnresolvedReferences
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.vtkCommonColor import (
    vtkNamedColors
)
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyLine
)
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersGeneral import vtkTableToPolyData, vtkTransformPolyDataFilter
from vtkmodules.vtkIOInfovis import vtkDelimitedTextReader
from vtkmodules.vtkIOXML import vtkXMLPolyDataWriter
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleTrackballCamera
from vtkmodules.vtkInteractionWidgets import vtkCameraOrientationWidget, vtkOrientationMarkerWidget
from vtkmodules.vtkRenderingAnnotation import vtkAxesActor, vtkScalarBarActor
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkColorTransferFunction,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer
)


def get_program_parameters():
    import argparse
    description = 'Edit data from a CSV file and visualise it.'
    epilogue = '''
    This program selects ECEF or UTM coordinates from the input file and:
       1) Visualises the resultant points and lines.
       2) Optionally creates a VTP file for further analysis.
       3) Optionally saves the edited file used to create the visualisation as a CSV file.
    If Geographic coordinates are selected, just the resultant CSV file is saved.
    '''
    parser = argparse.ArgumentParser(description=description, epilog=epilogue,
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('file_name', help='The CSV file containing the data.')
    parser.add_argument('-v', '--generate_vtp', action='store_true', help='Generate the .vtp file.')
    parser.add_argument('-o', '--out_csv_fn', default=None, help='The file name for the edited CSV file.')
    parser.add_argument('-p', '--path', default='.',
                        help='The path to be appended to the .vtp and optional .csv file')

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-e', '--ecef', action='store_true', help='Use ECEF coordinates.')
    group.add_argument('-u', '--utm', action='store_true', help='Use UTM coordinates.')
    group.add_argument('-g', '--geo', action='store_true', help='Use geographic coordinates (latitude/longitude).')

    args = parser.parse_args()
    return args.file_name, args.generate_vtp, args.out_csv_fn, args.path, args.ecef, args.utm, args.geo


def main():
    ifn, generate_vtp, ofn, sp, ecef, utm, geo = get_program_parameters()
    file_name = Path(ifn)
    if not file_name.is_file():
        print('Unable to read:', file_name)
        return
    pth = Path(sp)
    if not pth.is_dir():
        if pth.is_file():
            print(sp, ' must be a path')
            return
    pth.mkdir(parents=True, exist_ok=True)

    # Build the output paths.
    vtp_fn = Path(pth / Path(ifn).stem).with_suffix('.vtp')
    if ecef:
        vtp_fn = vtp_fn.with_stem(vtp_fn.stem + '_ecef')
    if utm:
        vtp_fn = vtp_fn.with_stem(vtp_fn.stem + '_utm')
    if ofn:
        csv_fn = Path(pth / Path(ofn).name)
        if not csv_fn.suffix.lower() == '.csv':
            csv_fn = csv_fn.with_suffix('.csv')
        if ecef:
            csv_fn = csv_fn.with_stem(csv_fn.stem + '_ecef')
        if utm:
            csv_fn = csv_fn.with_stem(csv_fn.stem + '_utm')
        if geo:
            csv_fn = csv_fn.with_stem(csv_fn.stem + '_geo')
    else:
        csv_fn = None

    # Create a dataframe from the csv file.
    df = pd.read_csv(file_name)
    # For ECEF coordinates, we want to look down from the zenith.
    # So calculate the mid-point of the latitude.
    lat_details = df['Latitude'].describe()
    lat_mid_pt = (lat_details['max'] + lat_details['min']) / 2

    # Create a temporary csv file with just the needed columns.
    tmp_dir = tempfile.gettempdir()
    if tmp_dir is None:
        print('Unable to find', tmp_dir)
        return
    tmp_path = Path(tmp_dir, f'tmp_{file_name.name}')
    # Note: The dataframe dfv is actually just a view of the original dataframe df.
    if ecef:
        dfv = df[['X(m)', 'Y(m)', 'Z(m)', 'Elevation(m)']]
    elif utm:
        # Duplicate the elevation column, this will become the z-coordinate when UTM is selected.
        df['Elev'] = df.loc[:, 'Elevation(m)']
        dfv = df[['Easting(m)', 'Northing(m)', 'Elev', 'Elevation(m)']]
    else:
        dfv = df[['Longitude', 'Latitude', 'Elevation(m)']]
    dfv.to_csv(tmp_path, index=True, index_label='Index', header=True)
    # Copy the temporary file (if requested).
    if csv_fn:
        shutil.copy(tmp_path, csv_fn.absolute())

    points_reader = vtkDelimitedTextReader()
    points_reader.SetFileName(tmp_path)
    points_reader.DetectNumericColumnsOn()
    points_reader.SetFieldDelimiterCharacters(',')
    points_reader.SetHaveHeaders(True)

    table_pd = vtkTableToPolyData()
    table_pd.SetInputConnection(points_reader.GetOutputPort())
    if ecef:
        table_pd.SetXColumn('X(m)')
        table_pd.SetYColumn('Y(m)')
        table_pd.SetZColumn('Z(m)')
    elif utm:
        table_pd.SetXColumn('Easting(m)')
        table_pd.SetYColumn('Northing(m)')
        table_pd.SetZColumn('Elev')
    else:
        # Remove the temporary file, it is not needed any more.
        tmp_path.unlink(missing_ok=True)
        print('Only ECEF or UTM coordinates can be visualised.')
        return
    table_pd.Update()

    # Remove the temporary file, it is not needed any more.
    tmp_path.unlink(missing_ok=True)

    poly_data = table_pd.GetOutput()
    # poly_data = transform_filter.GetOutput()
    # We use the elevation as the active scalars.
    poly_data.GetPointData().SetActiveScalars('Elevation(m)')
    elev_range = poly_data.GetPointData().GetScalars().GetRange()

    num_pts = poly_data.GetNumberOfPoints()

    poly_line = vtkPolyLine()
    poly_line.GetPointIds().SetNumberOfIds(num_pts)
    for i in range(0, num_pts):
        poly_line.GetPointIds().SetId(i, i)

    # Create a cell array to store the lines in and add the lines to it.
    cells = vtkCellArray()
    cells.InsertNextCell(poly_line)

    # Add the lines to the dataset
    poly_data.SetLines(cells)

    poly_data.Modified()

    transform = vtkTransform()
    if utm:
        # Scale the elevation.
        transform.Scale(1, 1, 1)
    if ecef:
        # Rotate the ECEF coordinates
        # into VTK coordinates so that on the screen:
        # Y points North, X points East and Z points up.
        transform.RotateX(-(90 - lat_mid_pt))
        transform.RotateY(0)
        transform.RotateZ(90 - lat_mid_pt)
    transform_filter = vtkTransformPolyDataFilter()
    transform_filter.SetInputData(table_pd.GetOutput())
    transform_filter.SetTransform(transform)
    transform_filter.Update()

    if generate_vtp:
        writer = vtkXMLPolyDataWriter()
        writer.SetFileName(vtp_fn)
        writer.SetInputConnection(transform_filter.GetOutputPort())
        writer.SetDataModeToBinary()
        writer.Write()

    colors = vtkNamedColors()
    colors.SetColor("ParaViewBkg", [82, 87, 110, 255])

    lut = get_diverging_lut('cool_warm')
    # lut = get_diverging_lut1('DarkRed', 'Gainsboro', 'Green')

    mapper = vtkPolyDataMapper()
    mapper.SetInputConnection(transform_filter.GetOutputPort())
    mapper.SetScalarRange(elev_range)
    mapper.SetLookupTable(lut)
    mapper.ScalarVisibilityOn()

    actor = vtkActor()
    actor.SetMapper(mapper)

    window_width = 1024
    window_height = 1024

    # Create a scalar bar
    scalar_bar = vtkScalarBarActor()
    scalar_bar.SetLookupTable(mapper.GetLookupTable())
    scalar_bar.SetTitle('Elevation')
    scalar_bar.UnconstrainedFontSizeOff()
    scalar_bar.SetNumberOfLabels(6)
    scalar_bar.SetVerticalTitleSeparation(50)
    scalar_bar.SetMaximumWidthInPixels(window_width // 8)
    scalar_bar.SetMaximumHeightInPixels(window_height // 2)
    scalar_bar.SetBarRatio(scalar_bar.GetBarRatio() * 0.6)
    scalar_bar.SetPosition(0.87, 0.1)

    renderer = vtkRenderer()
    ren_win = vtkRenderWindow()
    ren_win.AddRenderer(renderer)
    ren_win.SetSize(window_width, window_height)
    if ecef:
        ren_win.SetWindowName('ECEF')
    elif utm:
        ren_win.SetWindowName('UTM')
    iren = vtkRenderWindowInteractor()
    iren.SetRenderWindow(ren_win)
    style = vtkInteractorStyleTrackballCamera()
    iren.SetInteractorStyle(style)

    renderer.AddActor(actor)
    renderer.AddActor(scalar_bar)
    renderer.SetBackground(colors.GetColor3d('ParaViewBkg'))

    cam_orient_manipulator = vtkCameraOrientationWidget()
    cam_orient_manipulator.SetParentRenderer(renderer)
    cam_orient_manipulator.Off()

    axes = vtkAxesActor()
    axes.SetXAxisLabelText('East')
    axes.SetYAxisLabelText('North')
    # Zenith
    axes.SetZAxisLabelText('Zenith')

    widget = vtkOrientationMarkerWidget()
    rgba = [0] * 4
    colors.GetColor('Carrot', rgba)
    widget.SetOutlineColor(rgba[0], rgba[1], rgba[2])
    widget.SetOrientationMarker(axes)
    widget.SetInteractor(iren)
    widget.SetViewport(0.0, 0.0, 0.2, 0.2)
    widget.SetEnabled(1)
    widget.InteractiveOn()

    renderer.ResetCamera()
    renderer.GetActiveCamera().Elevation(0)

    iren.Initialize()

    ren_win.Render()
    iren.Start()


def get_diverging_lut(color_map: str, table_size: int = 256):
    """
    See: [Diverging Color Maps for Scientific Visualization](https://www.kennethmoreland.com/color-maps/)
                       start-point         mid-point           end-point\n
    cool to warm:     0.230, 0.299, 0.754 0.865, 0.865, 0.865 0.706, 0.016, 0.150\n
    purple to orange: 0.436, 0.308, 0.631 0.865, 0.865, 0.865 0.759, 0.334, 0.046\n
    green to purple:  0.085, 0.532, 0.201 0.865, 0.865, 0.865 0.436, 0.308, 0.631\n
    blue to brown:    0.217, 0.525, 0.910 0.865, 0.865, 0.865 0.677, 0.492, 0.093\n
    green to red:     0.085, 0.532, 0.201 0.865, 0.865, 0.865 0.758, 0.214, 0.233\n

    :param color_map: The color map to use e.g. cool_warm.
    :param table_size: The table size.
    :return:
    """
    color_maps = dict()
    color_maps['cool_warm'] = {'start': (0.230, 0.299, 0.754), 'mid': (0.865, 0.865, 0.865),
                               'end': (0.706, 0.016, 0.150)}
    color_maps['purple_orange'] = {'start': (0.436, 0.308, 0.631), 'mid': (0.865, 0.865, 0.865),
                                   'end': (0.759, 0.334, 0.046)}
    color_maps['green_purple'] = {'start': (0.085, 0.532, 0.201), 'mid': (0.865, 0.865, 0.865),
                                  'end': (0.436, 0.308, 0.631)}
    color_maps['blue_brown'] = {'start': (0.217, 0.525, 0.910), 'mid': (0.865, 0.865, 0.865),
                                'end': (0.677, 0.492, 0.093)}
    color_maps['green_red'] = {'start': (0.085, 0.532, 0.201), 'mid': (0.865, 0.865, 0.865),
                               'end': (0.758, 0.214, 0.233)}

    ctf = vtkColorTransferFunction()
    ctf.SetColorSpaceToDiverging()
    cm = color_maps[color_map]

    ctf.AddRGBPoint(0.0, *cm['start'])
    ctf.AddRGBPoint(0.5, *cm['mid'])
    ctf.AddRGBPoint(1.0, *cm['end'])

    lut = vtkLookupTable()
    lut.SetNumberOfTableValues(table_size)
    lut.Build()

    for i in range(0, table_size):
        rgba = list(ctf.GetColor(float(i) / table_size))
        rgba.append(1)
        lut.SetTableValue(i, rgba)

    return lut


def get_diverging_lut1(start: str, mid: str, end: str, table_size: int = 256):
    """
    Create a diverging lookup table from three named colors.

    :param start: The start-point point color.
    :param mid: The mid-point color.
    :param end: The end-point color.
    :param table_size:  The table size.
    :return:
    """
    colors = vtkNamedColors()
    # Colour transfer function.
    ctf = vtkColorTransferFunction()
    ctf.SetColorSpaceToDiverging()
    p1 = [0.0] + list(colors.GetColor3d(start))
    p2 = [0.5] + list(colors.GetColor3d(mid))
    p3 = [1.0] + list(colors.GetColor3d(end))
    ctf.AddRGBPoint(*p1)
    ctf.AddRGBPoint(*p2)
    ctf.AddRGBPoint(*p3)

    lut = vtkLookupTable()
    lut.SetNumberOfTableValues(table_size)
    lut.Build()

    for i in range(0, table_size):
        rgba = list(ctf.GetColor(float(i) / table_size))
        rgba.append(1)
        lut.SetTableValue(i, rgba)

    return lut


if __name__ == '__main__':
    main()
