# HOW TO RUN

Make sure you're running a GemRB built with PNG, OGG and FreeType support.

Copy either sample config from gemrb/ and set:
GameType=demo
GamePath to the parent of the parent dir containing this file (demo)
Width=800
Height=600

If you want to skip the intro windows for faster development, add also:
SkipIntroVideos = 1

## Shortcut for running from the build dir

Create the config:
```
cat > demo.cfg <<KUKU
GameType=demo
GamePath=../demo
GemRBPath=../gemrb
PluginsPath=./gemrb/plugins/
Width=800
Height=600
KUKU
```

Run: `gemrb/gemrb -c demo.cfg`

## Development

When adding new resources, please put them into `override/` and run
```
$ python tools/demo_key_file.py demo
```
from the project root directory. Add the updated `chitin.key` to the
commit stage.

While not necessary for GemRB, only this way makes the demo compatible for
exploring the resources with *Near Infinity*.

# AUTHORS

Authors are noted in the Debian-compatible `copyright` file.

For XCF and other files used to generate some of the used artwork, see the
[gemrb-assets repo](https://github.com/gemrb/gemrb-assets).

All used OpenGameArt assets are collated in this collection:
https://opengameart.org/content/gemrb-demo-assets

Main music theme and area music licensed from Nicolas Jeudy of Dark
Fantasy Studio. See https://github.com/gemrb/gemrb-assets/tree/master/demo/audio for the documents.
