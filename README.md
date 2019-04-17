# mrAiNoises
Additional noises for Arnold, complementing the procedural noise shaders provided by standard Arnold plugins and alShaders.

## Worley Noise
Provides a more expansive interface than alCellNoise, in particular non-euclidean distances, additional distance combination modes and gap computation.

Based on [miWorleyNoise](https://github.com/mruegenberg/miWorleyNoise) and [alCellNoise](http://anderslanglands.com/) and internally uses the Arnold API.

## Curl Noise
Originally based on [Simplex Noise](https://github.com/simongeilfus/SimplexNoise) but since switched to directly computing the curl of Arnolds noise function.

## Building
1. Put the Arnold SDK in `deps`
, then update the corresponding variable in `CMakeLists.txt`.
2. `mkdir build && cd build`
3. ``cmake .. -DARNOLD_BASE_DIR=`pwd`/../deps/Arnold-5.1.1.1-linux && make``
   (Replace the Arnold version with your own.)

This should result in shared libs (dll/so/dylib) in the `build` directory. Copy these, along with the `.mtd` files from `ui` to your `ARNOLD_PLUGIN_PATH`.

# Samples
A sphere displaced with mrAiCurlnoise and a mrAiWorleynoise texture showing the gap computation.

![sample](https://raw.githubusercontent.com/mruegenberg/mrAiNoises/master/samples/noises.jpg)

## License
The current version of the code is licensed under the MIT license.

Basically, you can use and modify this free of charge for private or commercial projects but may not remove copyright notices.

