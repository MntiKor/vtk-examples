# Building an example in WASM

## Building VTK

To build an example using WebAssembly, you can use the [kitware/vtk-wasm](https://hub.docker.com/r/kitware/vtk-wasm/tags)
docker image but some modules are disabled in that version that may be needed
for many examples.

To build VTK by yourself, you will need to follow the instructions in
[this documentation article](https://docs.vtk.org/en/latest/advanced/build_wasm_emscripten.html),
except that at the first step of the build, you will have to
change the configuration command to:

```
emcmake cmake \
        -S /work/src \
        -B /work/build-vtk-wasm \
        -GNinja \
        -DBUILD_SHARED_LIBS:BOOL=OFF \
        -DCMAKE_BUILD_TYPE:STRING=Release \
        -DVTK_ENABLE_LOGGING:BOOL=OFF \
        -DVTK_ENABLE_WRAPPING:BOOL=OFF \
        -DVTK_MODULE_ENABLE_VTK_cli11:STRING=YES \
        -DVTK_MODULE_ENABLE_VTK_RenderingLICOpenGL2:STRING=DONT_WANT \
        -DVTK_BUILD_TESTING=ON \
        -DCMAKE_INSTALL_PREFIX=/install
```

## Building the example

< Download the example placeholder >

To build your example, simply run:
```
emcmake cmake -DVTK_DIR=/work/build-vtk-wasm -S path_to_source -B path_to_build
cmake --build path_to_build
```

Then the only thing you have to do is to run the example:
```
cd path_to_build
python3 -m http.server 8000
```

Open your browser to http://localhost:8000 and enjoy !
