from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkIOImage import vtkDICOMImageReader
from vtkmodules.vtkInteractionImage import vtkImageViewer2
from vtkmodules.vtkRenderingCore import vtkRenderWindowInteractor
import vtkmodules.vtkRenderingOpenGL2


def get_program_parameters():
    import argparse
    description = 'Read Dicom image data'
    epilogue = ''''''
    parser = argparse.ArgumentParser(description=description, epilog=epilogue,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('filename', help='prostate.img')
    args = parser.parse_args()
    return args.filename



def main():
    colors = vtkNamedColors()


    inputFilename = get_program_parameters()

    # Read the DICOM file
    reader = vtkDICOMImageReader()
    reader.SetFileName(inputFilename)
    reader.Update()

    # Visualize
    imageViewer = vtkImageViewer2()
    imageViewer.SetInputConnection(reader.GetOutputPort())
    renderWindowInteractor = vtkRenderWindowInteractor()
    imageViewer.SetupInteractor(renderWindowInteractor)
    imageViewer.Render()
    imageViewer.GetRenderer().SetBackground(colors.GetColor3d("SlateGray"))
    imageViewer.GetRenderWindow().SetWindowName("ReadDICOM")
    imageViewer.GetRenderer().ResetCamera()
    imageViewer.Render()

    renderWindowInteractor.Start()

if __name__ == "__main__":
    main()
