# Experiment - FPS limiting

An extra toolbar button lets you limit how many frames are rendered for the rack UI every second. This might help alleviate some kinds of CPU issues, especially on older macbooks.

Various options are available from 144 down to 1 frame per second.
At 1fps, the UI is unusable and will automatically switch to 10fps when you press the FPS button again.

# Experiment - multi-threading

Make the existing engine multi-threaded.
Use the rocket ship icon to choose how many threads you want to use.

# Experiment - append-patch

Append an existing patch below your current patch.

Lets you build a library of components and compose them together.

Use the folder with a plus icon to access the append menu.
You can append a whole patch, or skip the first row of the patch you're appending.

The skip first row feature lets you build a component with audio and test trigger modules in the first row that don't get added to the combined patch when you append it.


# Experiment - wire-colours

This experiment lets you customise the available colours available and easily switch them while placing wire.

When you run this version of rack for the first time and close it, a new section will be added to settings.json.
This will be a list of all the wire colours you can choose from when patching up your rack.
You can modify this list by adding your own html style colours - delete or change the existing ones and add as many new colours as you like.
If it all goes wrong, delete the `wireColors` section entirely and you'll go back to the defaults when you next launch.

When placing a wire, if you right-click as you drag it, you can cycle the current wire through all the available colours.

There are many ways of finding the html value of the colours you want, just search the web for "html color picker" or something similar.

# Experiment - module pushing

Holding shift while moving a module left or right will push any other modules it hits out of the way.
This lets you push a bunch of modules together to easily make space for another module.

# Rack

*Rack* is the engine for the VCV open-source virtual modular synthesizer.

For information about the software, go to the [VCV website](https://vcvrack.com/) or the [VCV Rack manual](https://vcvrack.com/manual/).

## The [Issue Tracker](https://github.com/VCVRack/Rack/issues?q=is%3Aissue+is%3Aopen+sort%3Aupdated-desc) is the official developer's forum

Bug reports, feature requests, questions, and discussions are welcome on the GitHub Issue Tracker for all repos under the VCVRack organization.
However, please search before posting to avoid duplicates, and limit to one issue per post.

Please vote on feature requests by using the Thumbs Up/Down reaction on the first post.

I rarely accept code contributions to Rack itself, so please notify me in advance if you wish to send a pull request.


## Licenses

All **source code** in this repository is licensed under [BSD-3-Clause](LICENSE.txt) by [Andrew Belt](https://andrewbelt.name/).

**Component Library graphics** in `res/ComponentLibrary` are licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/) by [Grayscale](http://grayscale.info/). Commercial plugins must request a commercial license to use Component Library graphics by emailing contact@vcvrack.com.

**Core panel graphics** in `res/Core` are copyright © 2017 Grayscale. You may not create derivative works of Core panels.

The **VCV logo and icon** are copyright © 2017 Andrew Belt and may not be used in derivative works.

The **"VCV" name** is trademarked and may not be used for unofficial products. However, it is acceptable to use the phrase "for VCV Rack" for promotion of your Rack plugin. For all other purposes, email contact@vcvrack.com.
