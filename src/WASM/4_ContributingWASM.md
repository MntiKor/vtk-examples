# Adding WASM preview to an example

## Configure your example if it needs to be executed with arguments

First, if it requires files, you will need to ensure those files are stored in
__REPO_NAME__/src/Testing/Data with the following order:

- standalone files are in the root directory

- directories are inside another directory of the same name

- files which are dependent to a filesystem are in a directory with their
  whole file system (e.g. __REPO_NAME__/src/Testing/Data/PBR_Examples)

This is to ensure that file packaging by our CI pipeline is well done.
If you want to see what it should look like, take a look at
__REPO_NAME__/src/Admin/ArgsNeeded.json, especially examples like RayCastIsosurface
or PBR_Anisotropy.

Then, you will need to edit __REPO_NAME__/src/Admin/ArgsNeeded.json.
If you need a directory, remember it will be mapped to "/" in the
WebAssembly virtual filesystem (e.g. DicomTestImage/DicomTestImage/brain\_001.dcm
will become /DicomTestImage/brain\_001.dcm).
ArgsNeeded.json needs two variables for each example:
the args in the same order you would put them when running your example from bash
and the files your example will need to package within the WASM filesystem.
Leave the list empty if there's none.

## test your example

Run the script to generate the CMakeLists and the index.html you need:

```
cd __REPO_NAME__/src/Admin
./GenerateExampleWASM.sh path/to/example
cd path/to/example
```

Then build and run your example [as explained here](3_BuildingWASM).
If it works well, then you are finished here. If it doesn't because of
an error in VTK pipeline, then revert the changes you made to ArgsNeeded.json
and add the example to the exclusion list. You are welcome to add an issue
to [VTK Gitlab](https://gitlab.kitware.com/vtk/vtk/-/issues) if you think it's
relevant.
