# mrAiNoises
Additional noises for Arnold, complementing the procedural noise shaders provided by standard Arnold plugins and alShaders.

## Worley Noise
Provides a more expansive interface than alCellNoise, in particular non-euclidean distances, additional distance combination modes and gap computation.

Based on [miWorleyNoise](https://github.com/mruegenberg/miWorleyNoise) and [alCellNoise](http://anderslanglands.com/) and internally uses the Arnold API.

## Curl Noise
Based on [Simplex Noise](https://github.com/simongeilfus/SimplexNoise) by Simon Geilfus.

## Building
1. Put the Arnold SDK in `deps`, then update the corresponding variable in `CMakeLists.txt`.
2. `mkdir build && cd build`
3. `cmake .. && make`

This should result in shared libs (dll/so/dylib) in the `build` directory. Copy these, along with the `.mtd` files from `ui` to your `ARNOLD_PLUGIN_PATH`.

## License
The code itself is licensed under the BSD license and the MIT license. 
SimplexNoise (the main dependency) is licensed under the BSD license and uses GLM, which is licensed under the MIT license.

Basically, these licenses all allow commercial use for free provided some conditions are met if you redistribute the software.
