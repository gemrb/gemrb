# HOW TO RUN

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

# AUTHORS

If not otherwise noted, the author of the files is GemRB team and they are
available under GPL2+ (code) like GemRB itself. Art assets under CC-BY-SA 4.0.

For XCF and other files used to generate some of the used artwork, see the
[gemrb-assets repo](https://github.com/gemrb/gemrb-assets).

The background graphic for ar0100 was contributed by Aranthor (made from
scratch) and is available as-is, requiring only attribution.

The avatar animation was contributed by deepinthewoods.

The big avatar animation was contributed by Copyright 2010-2012 Jochen
Winkler <http://www.jochen-winkler.com/> under GPL3 or CC-BY-SA3.

The Eadui font was made by Peter S. Baker and is available under the OFL (SIL
Open Font License). Lato is also available under the OFL.

Portrait image "fan-art Boromir 'Lotr'" by Deevad (CC BY 3.0).

## OpenGameArt

All used OpenGameArt assets are collated in this collection:
https://opengameart.org/content/gemrb-demo-assets

Hand cursor icon was done by yd (CC0).

Window and GUI design based on Lamoot's RPG GUI construction kit v1.0 (CC-BY)
World map by Pepper Racoon (CC-BY).

Ruby image by rubberduck and Clint Bellanger, based on designs from Justin Nichol (CC-BY 4.0).

Swirl by Beast (CC0), temporarily selectively dithered with imagemagick.
Loading spinner by qubodup (CC0).

Ground pile container image and inventory gold image by Clint Bellanger (CC-BY-SA 3.0).
Backpack image by mold and Ravenmore (CC-BY 3.0). Pile pick-up sound under
(CC-BY-SA 3.0) by Hansjörg Malthaner (http://opengameart.org/users/varkalandar).

Inventory slot item backgrounds collected by lukems-br (CC-BY 3.0) plus
from Game-Icons.net: quiver icon by Delapouite (CC-BY 3.0) and bag icon by Lorc (CC-BY 3.0).

Button press sound by Kenney.nl (CC0), window open close sounds by rubberduck (CC0).

Footstep sounds and portal destruction sfx by Little Robot Sound Factory
(www.littlerobotsoundfactory.com; CC-BY 3.0).
