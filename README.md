# Experiment - wire-colours

This experiment lets you customise the available colours available and easily switch them while placing wire.

When you run this version of rack for the first time and close it, a new section will be added to settings.json.
This will be a list of all the wire colours you can choose from when patching up your rack.
You can modify this list by adding your own html style colours - delete or change the existing ones and add as many new colours as you like.
If it all goes wrong, delete the `wireColors` section entirely and you'll go back to the defaults when you next launch.

When placing a wire, if you right-click as you drag it, you can cycle the current wire through all the available colours.

There are many ways of finding the html value of the colours you want, just search the web for "html color picker" or something similar.

# Free Rack

*FreeRack* is a free derivative version of the original open-source virtual modular synthesizer by VCV.

This version is not officially supported in any way, do not contact VCV about issues related to this version of the software.

## Licenses

**original source code** in this repository is licensed under [BSD-3-Clause](LICENSE.txt) by [Andrew Belt](https://andrewbelt.name/).

**Component Library graphics** in `res/ComponentLibrary` are licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/) by [Grayscale](http://grayscale.info/). Commercial plugins must request a commercial license to use Component Library graphics by emailing contact@vcvrack.com.

**Core panel graphics** in `res/Core` are public domain works.
