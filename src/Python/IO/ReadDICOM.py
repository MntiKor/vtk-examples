from vtk import vtkDICOMImageReader,vtkImageViewer2,vtkRenderWindowInteractor,vtkNamedColors


def get_program_parameters():
    import argparse
    description = 'Read Dicom image data'
    epilogue = ''''''
    parser = argparse.ArgumentParser(description=description, epilog=epilogue,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('filename', help='vase.vti')
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
    imageViewer.SetSize(1000,1000)
    renderWindowInteractor = vtkRenderWindowInteractor()
    imageViewer.SetupInteractor(renderWindowInteractor)
    imageViewer.Render()
    imageViewer.GetRenderer().SetBackground(colors.GetColor3d("black"))
    imageViewer.GetRenderWindow().SetWindowName("ReadDICOM")
    imageViewer.GetRenderer().ResetCamera()
    imageViewer.Render()

    renderWindowInteractor.Start()

if __name__ == "__main__":
    main()
