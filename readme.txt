NOTE: Below "WARNINGS" starts the sections on how to play the game and
navigate the menus and such. Below that is a small Q/A.
THIS GAME IS UNFININSHED!!!!! THERE WILL BE BUGS!!!

===============================
LICENSE
===============================
There is no license (so far), just spread the word about this game please!

===============================
I DID NOT MAKE THE MUSIC
===============================
I used lots of music from different places, so none of the music is made by
me.
For a quick summary of the music used, Doom midis, Sonic midis, Misc. midis,
and more were used.
Most sound effects were kinda made by me by combining multiple things off of
youtube and other video games. For instance the AWP weapons sfx is just a copy
of the AWP sfx from CS:S.

===============================
WARNINGS
===============================
WARNING: Some levels will crash the game and some don't work (because of game
level format changes and such)!!!
Most will open, but a lot are unfinished and unbalanced considering the
balancing changes that are being done so often.
There are a few more levels i'd like to add in because other people have made
them, but currently I don't have their level files.

===============================
HELP/HOW TO PLAY
===============================
MENUS:
The menus are easy to get used to, and aren't that hard except for a few
things to get used to.
When changing a string (so like text and stuff) or a number in the menus,
you have to hit <enter> to start editing the field and <escape> to stop
editing it. You will know that you are editing it when the menu cursor turns
blue instead of yellow. If you are in a sub-menu, you can just hit <escape> to
go back to the other menu you came from.
In a number field you can use 1 arithmatic operation like *,/,+,or- to
easily do math in a field. You can also use in integer fields "s" and "m"
prefixes on numbers when you're not doing math to automatically convert the
number of seconds/minutes into the number of frames for that thing.

HOW TO CONNECT TO OTHER PLAYERS:
To connect to another server, you have to know the IP, because the server
browser isn't implemented yet (There are plans to add file downloading,
especially because its coded in, just a few game breaking bugs are present in
the file sharing code). Just hit "Join Game Specify IP" and input the IP and
hit join game.

HOW TO HOST A GAME:
To port forward for this version, just forward UDP port 20291.
(In later versions you might also need to forward TCP port 17475)
After you've port forwarded, just start up a game in a level and you're off!
If you are playing a game with mods (ex: custom levels, graphics, or audio)
then make sure that players connecting also have the same mods installed,
otherwise it will cause desyncs.

MULTIPLAYER GLITCHES:
	(will try to be fixing sorry (they don't happen too often):
If you join a multiplayer game, there is a small chance that nothing will
spawn in and you can't see any other players, this means that the server
disconnected you for somereason. Disconnect and retry until it works.

TUTORIAL:
Just shoot with <down> jump with <up> move with <left> and <right> and
drop/pickup things with <space>.
You can talk with <t> and say it with <enter>.
You can bring up the pause menu with <escape>.
You can bring up the console with <tilde>.
You can also play the game with a controller!
Basically shoot enemies, dont die, and get to the end. Its all pretty simple
but its really fun to play with freinds and have a good server going.
Especially if you're playing a map you made, or a really good one (which there
aren't really many good ones right now).

COMMANDS:
save_blocks
load_blocks
set_block
full
say
exit
start_blockedit
start_mainmenu
clear_all_blocks
start_bp_edit
save_spawners
load_spawners
set_light
debug_hitboxes
debug_showobjgrid
debug_showgridposes
connect
print_connected_players
print_wobj_net_size
hires
cl_interp
showpos
cl_smoothing
cl_avg_frames_per_update
tunes
kill
show_nodeuuids
start_lightedit
noclip
get_checksum
load_gfx_file
load_audio_file
tcpinfo
set_strength
set_health
volume
set_speed
set_jump
set_item
set_upgrade
start_dialoge

===============================
HOW TO MOD
===============================
I tryed to make the game modding freindly, and am making more things in the
future like the ability to make custom enemies and items, but currently there
are 3 things you can mod normally: Levels, Graphics, Sounds, and Music.
P.S: I did try to make it so you could mod the game, but the level editor was
kinda needed either way because I couldn't make levels otherwise.

===============================
HOW TO MOD: LEVELS
===============================
Level files are split into 2 very simple formats for the game: .CNMB files and
.CNMS files.

.CNMB files house the data for the level layout in blocks, light, block
properties, and backgrounds.
.CNMS files house the data for the level objects, and ending text.

To start making a level first go to the main menu, then goto the options
sub-menu and turn on the level edit cheat.
Then go back and enter into the Block Properites Editor (see that section for
more info).
Then save and go back to the main menu to enter into the block editing mode.
(see that section for more info).
Then save and go back into the object editing mode and place in some spawns
and other nessesary objects for a level. (see that section for more info).
And BOOM! You're done that is the minimal amount of work you'd need to do to
create a level. More details on each editor are down below because they do
need some getting used to as they are not the most user-freindly saddly. Happy
Mapping!

===============================
HOW TO MOD: GRAPHICS, SOUNDS, AND MUSIC
===============================
To mod graphics copy the gfx.bmp and rename it to whatever you want.
Then start editing it. Since the file is in the 8-bit bmp format, you must
need an editor that can edit that type of file. I find that GraphicsGale is an
amazing program for that use and is pretty much made for that kind of thing.
For the 8 bit pallete you can change it around as the transparency tables and
such are generated at run-time in the game, but remember that changing the
pallete will change every other thing in the game that uses that color so
watch out.
To use your new texture pack or anything in the game type
"load_gfx_file <path to graphics file>" into the console.
To mod sounds just replace a uncompressed unsigned 8-bit 8000Hz .WAV file
into the sounds folder, or the mods/sounds folder.
While you can't make new sounds in the game, you can add in new music!
To make new music add a new .mid file into the music folder or the mods/music
folder. To hear the new music in the game, add its ID into the audio.cnma file
or create a new audio.cnma file to put it into.
IDS are created in the MUSIC section on a new line like:
<UNIQUE ID> <PATH TO .MID FILE>

===============================
BLOCK PROPERTIES EDITOR
===============================
The main goal of the block properties editor is to create tiles or "blocks"
for use within the main map geometry.
To start select block prop 1. I wouldn't use block prop 0, since usually thats
used as an "air" block because maps are automatically cleared with block type
0 when making a new map. When making a block you can edit multiple things.
Transparency (0 is opaque, 7 is fully transparent).
Damage Types:
	None: Will never damage the player
	Lava: Acts like water giving the player inifinte jumps, and making
		the player move more slow.
	Spikes: Just damages the player.
Damage: how many hit points this block deals to the player per frame.
Solid: Whether or not the block can be stood on by players and enemies.
Collision Type:
	Hitbox: The solid part of the block is defined by a box that can be
		changed when you enter into the collision data sub-menu.
	Heightmap: The solid parts of the block snap players and entities
		up above that part so it looks like they are walking on it.
		The solid parts of the block are made up of a line for every
		pixel column of the block so 32.
Animation speed: The number of frames that each frame in the animation has.
Number of frames: The number of frames in the animation
Find Bitmap: You may search through the graphics of the game for a block
in-game.
When you're done MAKE SURE TO SAVE IT AS *.CNMB!!!

===============================
BLOCK EDITOR
===============================
First load the block props from your *.CNMB file
Press escape to switch between the menus and edit mode.
Use the arrow keys to move around.
Then you can use A and D to scroll through the block selection of your
.CNMB block properties file.

The 2 block layers:
In CNM Online there are 2 block layers that dictate the order of drawing the
level graphics when playing the game.
	- Blocks in the background layer will be drawn below objects but can
	still be solid.
	- Blocks in the foreground layer will be drawn above objects and can
	be non-solid.
To place the blocks in these 2 modes you use different keys to erase and place
them to both.
Use <W> to place a foreground block, and <S> to remove a foreground block.
(AKA foreground block editing is on the left side of the keyboard)
Use <Space> to place a background block and <Enter> to remove a background
block.
MAKE SURE TO SAVE TO YOUR *.CNMB FILE WHEN YOU'RE DONE!!!

===============================
LIGHT EDITOR
===============================
This editor is the same as the block one, except that you can only use
<W> and <S> to set light values.

===============================
BACKGROUND EDITOR
===============================
First, load your *.CNMB file.
By far the most in-depth editor, there is a lot to learn on this one.
Backgrounds in the game are made up of a certain amound of layers.
When playing a level, there may be only (for example) 6 layers shown out of
the possible 32. This is to allow switching backgrounds on the fly in levels.
To change what layers are visible in the preview just change the visible 
layers end and start numbers. To preview the backgrounds just select "preview
 background" and escape to get out of the preview. You can use A,W,S, and D to
move faster around the level.

Now to start editing a background layer.
Layers further down the list will draw over layers further up like 0, and 1.
	ORIGIN: This is the base position of the layer in the world.
	SCROLL: How slow background will scroll. Higher values will make it
		seem farther away.
	SPEED: How fast the background moves on its own in pixels per frame.
	SPACING: How spaced apart each bitmap is in pixels.
	REAPEAT METHODS: How the bitmap is repeated.
	CLEAR COLOR: Leave this to 0, if you will use a bitmap for the layer,
		otherwise use an index into the graphics file's 8bit color
		palette for a color to clear the screen with.
	BITMAP POSITION: The position in the graphics file of the bitmap.


MAKE SURE TO SAVE TO YOUR *.CNMB FILE WHEN YOUR'RE DONE!!!

===============================
OBJECT PLACEMENT (SPAWNER) EDITOR
===============================
MAKE SURE TO LOAD YOUR *.CNMS FILE IF YOU SAVED ONE EARLIER

First load your .cnms spawner file. You may optionally load the map geometry
.cnmb block file to see how the level looks, but this is not required.
Use escape to switch between edit mode and the menu.
Use <W> and <S> to scroll through the object selection.
Use <A> and <D> to speed up the movement speed.
Use the arrow keys to move around.
Use <Enter> to edit a spawners properties.
Use <Backspace> to delete a spawner.
Use <Space> to place a spawner.

The number above an spawner shows how long it takes before it will respawn.
When there is no number, it makes the object will never respawn.
If the number is instead "R-I", it will infinitly respawn instantaniously.

How to place: Teleports:
	To place a teleport, go back to the menu and toggle spawn
	teleporters ON. From there you may change the name and the cost that
	it takes to use the teleporter. Then exit the menu and place the
	spawner wherever you want with space. In the top left corner it will
	then say "PLACE TELEPORT LOCATION", so now hit space where you want
	the teleport to take you. TADA!! Your teleport has now been made!
	You can go back and edit any properties of it that you want when you
	hit <Enter> on it! Just make sure to exit teleport mode to place other
	object types!

How to place: Everything else:
	When you hit space to place a spawner you may hit enter to see and
	edit its many properites.
	X: The x position of the object
	Y: The Y position of the object
	*COPY VALUES: This may only pop up for certain objects because for
		some objects it is not safe to directly copy values.
	SPAWN TIMER: How many frames in between respawns of the object
	MAX SPAWNS: How many objects from this spawner are allowed in the game
		at one time?
		For example a max spawns of 3 will allow 3 objects spawned in
		at once and will spawn in another object once the last was
		destroyed.
		A Max Spawns of 0 means it will never respawn.
	SPAWN MODE:
	SINGLE AND MULTI-PLAYER: This object will spawn in both single and
		multiplayer modes.
	SINGLEPLAYER ONLY: This object will only spawn in singleplayer mode.
	MULTIPLAYER ONLY: This object will only spawn in multiplayer mode.
	BASED ON PLAYER COUNT: This object will spawn X objects every respawn
		based on the number of players in the game. So for example
		a slime spawner of this type in a 6 player game will spawn
		6 slimes every time the last 6 die.
	CUSTOM INT/THE CUSTOM INTEGER VALUE OF THE OBJECT:
		This field will change name based on the object of choice,
		so look at other examples of the useage of the field in other
		levels and the name of the field to get an idea of what it
		does.
	CUSTOM FLOAT: Same as custom int, but with a decimal value instead.
	DROPPED ITEM: The item that is dropped when this object dies.
		An item ID of 0 means drop nothing. You may use the search
		for dropped item button to look for an item.

Thats it for the spawner editor.
MAKE SURE TO SAVE TO YOUR *.CNMS FILE BEFORE EXITING!!!

===============================
ENDING TEXT/DIALOGE/STRING EDITOR
===============================
This editor is used for many different purposes for the objects in CNM,
like ending text spawners, dialoge spawners, and graphics changing objects.
MAKE SURE TO LOAD YOUR SPAWNER .CNMS FILE BEFORE EDITING ENDING TEXT AND 
SAVING IT BACK TO THE SPAWNER FILE!!!
Here just hit enter on any line index and start typing!
MAKE SURE TO SAVE TO YOURE .CNMS FILE BEFORE EXITING!!!

===============================
MISC MODDING DOCUMENTATION
===============================
Max Power Upgrade Ability list:
0: Nothing
1: Double Jump
2: Flight (like normal wings power up)
3: Jumping Shield (when you land after pressing jump in mid-air, you gain a shield for a small amount of time)
4: You bounce off of enemies and things that hurt you (besides hurt tiles)


===============================
FUTURE PLANS
===============================
I plan on selling this game, but definently not this alpha version, while its
a good test and a good concept, better exceution is needed, on the coding
front, game design front, and pretty much everything else. Plus, even though
I do think this game already has a good identity, I think I can make it even
more distinct to my sort of style and still be more palettable for normal
people who don't like crusty games, y'know?
So for the final game, a total code rewrite, graphics overhaul, and level
structure overhaul are gonna be done, but this will be I think akin to
something like Minecraft Classic where its a good game and a great start
but not good enough to be sold.

I also want to make a 3D sequel after this one which will be pretty cool, I'm
also thinking that it should be on the Sega Dreamcast, but the sequal is
something that will be made wayyyy down the line.

===============================
THE ORIGINS OF CNM ONLINE
===============================
CNM Online might be a little weird, but thats because it wasn't even meant to
be the game it is right now. CNM Online is actually a sequel to an older game
I made with my cousins, just called CNM. My cousins also made some of the
levels in this version of the game! So, I am thinking of releasing the
original CNM because of its position of being the first game in the series,
but just telling you its not really fun without other people. The main reason
this version of the game is online multiplayer is because the original game
was really fun to play around in with mu cousins on its local 4 player co-op,
so I thought it would be awesome to make a online version of it, so here we
are!

===============================
LEVEL CREATORS
===============================
ARID CANYON - TYLER
*bf.cnmb - TYLER
*bg.cnmb - TYLER
CASIMOE - TYLER,SUMRAD7
chester_city.cnmb - SHADOWREALM9
deep_ocean.cnmb - SHADOWREALM9
deephaus.cnmb - SUMRAD7
dropper.cnmb - SHADOWREALM9
EASTER ISLAND - SHADOWREALM9
et.cnmb - SHADOWREALM9
*greenhill.cnmb - SHADOWREALM9
KUMO - THE CLOUDS - SUMRAD7
lavaland.cnmb - JAYLEN
*legacy_city.cnmb - SHADOWREALM9
*modern_city.cnmb - SHADOWREALM9
ocean.cnmb - TYLER
RUN.cnmb - TYLER
SUPER CITY - SHADOWREALM9
*super_city.cnmb - SHADOWREALM9
*super_die.cnmb - SHADOWREALM9
tester_city.cnmb - SHADOWREALM9
THEdeephaus.cnmb - SHADOWREALM9
themine.cnmb - JAYLEN
thewild.cnmb - TYLER
train.cnmb - TYLER
tt.cnmb - TYLER
wind_test.cnmb - SHADOWREALM9

* Don't work in the current version.

===============================
CONSOLE COMMANDS
===============================
save_blocks
load_blocks
set_block
full
say
exit
start_blockedit
start_mainmenu
clear_all_blocks
start_bp_edit
save_spawners
load_spawners
set_light
debug_hitboxes
debug_showobjgrid
debug_showgridposes
connect
print_connected_players
print_wobj_net_size
hires
cl_interp
showpos
cl_smoothing
cl_avg_frames_per_update
tunes
kill
show_nodeuuids
start_lightedit
noclip
get_checksum
load_gfx_file
load_audio_file
tcpinfo
set_strength
set_health
volume
set_speed
set_jump
set_item
set_upgrade
start_dialoge
debug_state
set_money

===============================
THANKS
===============================
Thank you so much for downloading CNM Online, I hope its a good time!
