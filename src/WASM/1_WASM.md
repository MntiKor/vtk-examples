# The "View Live" option (only available for Cxx examples)

## What is WebAssembly ?

[WebAssembly](https://webassembly.org/) (WASM for short) is a technology which
allows a C/C++ code to be executed by the web browser as if it was a JS script.

It allows integration of small bits of code inside a web page,
and even full size web apps that are derived from a desktop app.

For more examples of what WASM is able to do, take a look at
[Made with WebAssembly](https://madewithwebassembly.com/).

## VTK-WASM

By making VTK able to use WebGL instead of the usual OpenGL,
we have been able to build VTK for WebAssembly. Not all modules are available
but we keep on improving VTK-WASM, and this website is a display of what is
working, and what is not.

If you want to build your own VTK project in WebAssembly, take a look [here](3_BuildingWASM).

If your browser is having issues loading or running WASM examples, ensure
Hardware Acceleration is enabled, and if you have a GPU, that it's in use.
Be aware that some examples can be highly resource intensive and therefore not run
well in small hardware configurations (thus provoke instability for your browser).
VTK-WASM is still experimental so it could have issues. Feel free to report these
to [VTK Gitlab](https://gitlab.kitware.com/vtk/vtk/-/issues) so we can keep on improving
stability and support for VTK-Examples-WASM.
