# Changelog

## 0.3.7

### Fixed
- **Sound turning metallic after a while** (#18, #13; Linux and Steam Deck, after 5 to 25 minutes): once the sound card
  had run a little ahead of the PC, the audio was mixed in bursts, and the game's own sound effects, which it works
  out between two mixes, could not keep up. Mixes now stay evenly spaced, as TotK Recomp does, and Linux keeps a little
  more audio queued (60 ms).
- **Crash at the end of the first mission** on a new save (#16).
- **Button prompts after a remap** (#17): when A, B, X or Y is remapped, the prompts show the button that now does the
  action (from the next start).
- **Steam Deck plugged into a TV or a monitor**: the game now runs in docked mode there, with its sharper picture.
- **Linux update**: an update unpacked over an older folder now always rebuilds the game (it could keep the old one).

### Changed
- The game's log notes once a minute any audio problem it saw (skipped audio, starved sounds, late mixes).

## 0.3.6

### Fixed
- **Sound turning choppy and robotic after a few minutes** (#13, seen on Steam Deck and Linux): when the sound card
  plays slightly faster than the PC's clock, the queued audio slowly ran out and, after 4 to 5 minutes, every bit of
  sound came with a gap. The audio now follows the sound card's pace when it falls behind. The log also notes, once a
  minute, any audio that went missing.

## 0.3.5

### Fixed
- **Endless loading screen**: starting a mission could, rarely and at random, stay on the black loading screen
  forever (fine after a restart). The game's job system could leave a loading job queued with all its workers asleep;
  a worker that sleeps for a quarter of a second is now woken to check for one.

## 0.3.4

### Fixed
- **Magnesis** (#8): a caught object now stays in front of Link at every frame rate. At 60 fps it flew off and
  disappeared, above 60 fps it shook in every direction and could freeze the game.
- **Stasis above 60 fps**: casting it could freeze the game.

### Added
- **5.1 surround sound** (experimental): a new setting in the Sound tab sends the game's six audio channels to a 5.1
  system. It is off by default and applies after a restart.

### Changed
- **Settings menu**: every setting has a shorter, clearer description, in English and French.
- **RecompLauncher settings**: the same descriptions as the menu, and the 5.1 setting.

## 0.3.3

### Fixed
- **Build stopping at "Unpack the game data" with a cartridge dump** (#12): a base game in XCI format passed the check,
  then was refused. XCI now works at every step, and choosing a `.7z`, `.zip` or `.rar` says to unpack it first.

- **Title screen above 60 fps** (#10): Continue played the camera zoom twice and never reached the world map. It now
  plays once, without a jump, and goes to the map at every frame rate. The title screen itself runs at 60 fps.
- **Cutscene cameras above 60 fps**: a camera timeline could step past the moment that starts its next part. The
  take-off at the start of a mission now ends, and Link no longer shakes during it.

### Added
- **Compressed dumps**: NSZ and XCZ files are accepted. The setup unpacks them and checks every file against its hash.

### Known issues
- **Magnesis** (#8) only works correctly at 30 fps. At 60 fps a caught object flies off and disappears, and above 60 fps
  the rune can freeze the game. Switch to 30 fps with L3 + R3 before using it.

## 0.3.2

### Fixed
- **Build stopping at "Install the build tools" with "could not lock database"**: an MSYS2 install that was interrupted
  left a lock file behind, and every later build stopped on it. The setup now removes that lock when no `pacman` is
  running, on Windows and in the private WSL distro.
- **An interrupted or outdated MSYS2**: a first install that was cut off now resumes, and an MSYS2 that cannot get the
  packages is left alone while the build uses its own copy.
- **Busy download servers**: downloads and packages are tried again after a pause instead of stopping the build.
- **A `PYTHONHOME`, `PYTHONPATH` or inherited `PATH`** on the PC no longer breaks MSYS2's Python.
- **Update with the game open** now says to close it, instead of failing with a file error.
- **Drives**: FAT32 is refused at the start with a clear message, exFAT counts the extra 11 GB it needs, a network path
  is refused, and MSYS2's drive is checked for free space.
- **A shader preparation failure** (a graphics driver crash) no longer fails the build: the game works and prepares
  them while it plays.
- **`prod.keys.txt`** (saved by Notepad with the extension hidden) is accepted.

## 0.3.1

### Fixed
- **Build stopping on a PC that already has MSYS2** (`DLL load failed while importing binascii`): the build tools step
  now checks that MSYS2's Python can load what the build needs. When it cannot, that MSYS2 is left untouched and the
  build installs its own copy in `C:/AgeOfCalamityRecomp-tools/msys64` (about 1.5 GB).
- **The game window comes to the front** when it opens, instead of staying behind the other windows. If Windows
  refuses (you moved to another program during the start), its taskbar button flashes.
- **Aiming**: the left stick no longer moves the aim of the bow and the other aimed actions, only the right stick does.
  **Aim with the left stick** (Controls tab, and in RecompLauncher; off by default) brings the game's original aiming
  back, at once.
- **A `profile` folder that is a junction or symbolic link** (saves kept on another disk) no longer stops the game at
  start with "cannot create directory".

## 0.3.0

### Added
- **Dynamic resolution setting** (Screen tab, and in RecompLauncher): in battles the game draws the 3D scene smaller
  for a moment when frames run late; in a charge across Hyrule Field at 60 fps it went down to 70 % of its height for
  a few seconds, a softer picture. **Off (full size)** keeps the scene at its full size every frame (patch set
  `dynres`); **Game** (default) leaves it to the game. Applies at once. Clearing the game's enable bytes instead, as
  the community "Locked" cheats do, held a battle's loading screen forever.
- **Each file of the dump can be chosen on its own**: the setup asks for one folder with everything, or, with "I
  choose each file on its own", for the base game, the update, `prod.keys` and the DLCs one by one, wherever they
  are (another disk, separate folders). The choices are kept for the next build. The build tools take the same
  files with `--nsp <file>` (repeatable) and `--keys`, with or without `--game-dir`.
- **The setup shows the time left** ("about 12 min left") next to the step, once the build has measured the PC for a
  minute: each step's expected time, scaled by how fast this PC went on the steps done so far (steps with nothing to
  do are left out). RecompLauncher's prepare step gets it in its progress text too. The bar no longer stands still
  for minutes: the RomFS merge reports the bytes hactool has written, and the steps that print no progress move with
  their expected time (up to 90 % of the step).
- **AMD FSR 3.1**: the DLSS / DLAA setting now also works on AMD and Intel graphics cards, on NVIDIA cards without
  DLSS, and on Linux and the Steam Deck (built there, not yet tried on a Linux device; DLSS stays Windows only). A new **Upscaler** row (Screen tab, and in RecompLauncher) picks it: **Automatic** (default)
  takes NVIDIA DLSS where the card and `nvngx_dlss.dll` have it, else FSR 3.1; **NVIDIA DLSS** or **AMD FSR 3.1**
  force one. **FSR 3.1 sharpness** (0-100 %, default 80, applies at once) sets its own sharpening.
  FSR 3.1 takes the same picture, depth, motion vectors and jitter as DLSS, in every mode: Native (DLAA)
  anti-aliases the scene at full size, Quality, Balanced and Performance draw it smaller and scale it up. In a
  Hyrule Field battle at 1080p and 60 fps it costs the same as DLSS. The AMD FidelityFX SDK's upscaler (MIT) is built
  into the game, as in TotK Recomp.
- **Gyro calibration** (Controls tab, **Calibrate gyro**): put the controller down, press, and leave it still for two
  seconds. The gyroscope's resting drift is measured and taken off every reading, so the aim no longer creeps to the
  side; the result is kept in `settings.ini` for that controller and loaded again when it connects. A controller that
  moved during the measure is refused. **Gyro calibration** turns the correction off without losing it.
- **Language tab and a French menu**: the settings menu (F1) gets a Language tab with **Menu language** (Automatic,
  English, Français; applies at once) and **Game language**, moved there from the Game tab. Automatic shows the menu in
  the system's language when it is translated, else in English. The whole menu and the performance overlay are in
  French; a text not translated yet shows in English.
- **`ACR_CRUMB_DRAWS=1` works with the recorder thread on** (it needed `ACR_VK_RECORDER=0` before): after a lost GPU
  device the log names the shaders of the draw the GPU was in. A diagnostic only: it slows the GPU a lot.

### Fixed
- **Crash when a mission starts** (issue #5, "vk: upload submit failed (-4)", seen on the Daruk mission): one kind
  of indirect draw skipped the thread that records the GPU commands in order, so it ran before its render pass and
  bindings and the GPU wrote to an invalid address. It now goes through the recorder like every other draw.
- **Linux: "the build stopped at Install the build tools" on Bazzite** (and other Fedora Atomic systems: Silverblue,
  Kinoite, Bluefin), whose `dnf` refuses to install anything. There, and on SteamOS (which the setup refused until now),
  the build tools now go into a distrobox container, `acr-build` (Arch Linux), which shares your home folder; the
  compile and the other steps that need the tools run inside it. Without distrobox the setup says so.
- **Windows build stopped after the game code with "DLL load failed while importing _rust"** (issue #6): the RomFS
  index and the shader preparation read the already decrypted game data, but imported the decryption library
  (`cryptography`) anyway, and an MSYS2 whose `cryptography` cannot load its DLL (an MSYS2 updated only in part)
  stopped there. They no longer need it.
- **No "Report a problem..." button when the build failed** (issue #6): the setup had it only once the game was
  installed. It now appears next to "Check files" as soon as a build stops, and the error box points to it.
- Gyro aiming: held still, the controller's "down" is put right within a second (it took about two), so an aim up or
  down no longer turns the camera to the side as well after a quick move; and the motion starts from the controller's
  first real reading instead of from a sensor that has not reported yet. Both from TotK Recomp.

## 0.2.1

### Fixed
- **"hactool could not open base.nsp" while the files check said all was good**: at each start hactool also reads
  the key files in the player's own `.switch` folder (`%USERPROFILE%\.switch`, `~/.switch`), left there by other
  Switch tools, and stops on a line it rejects (Lockpick's `master_kek_source` with an extra `00`, a key of another
  length, a rights ID twice in `title.keys`) before opening anything. The check reads the `prod.keys` of the game
  folder, so it saw nothing wrong. Every hactool run of the setup now uses the keys of the game folder only. Each NSP
  is checked against its file table once extracted (hactool ends with code 0 when a file could not be written, e.g.
  a full disk), a run cut short extracts again, and hactool's output goes to `build/v130/extract-base.log` (key
  values hidden), in the diagnostic report too, with its last line in the error.

## 0.2.0

### Fixed
- **World map cursor twice as fast at 60 fps** (issues #1 to #3): the cursor moved by a fixed step per game update,
  so at 60 fps it crossed the map in 2 seconds instead of 5. Its step now follows the frame rate (patch set `cursor`,
  `ACR_PATCH_CURSOR=0` turns it off). The same holds for the other menus that move a pointer with the stick.
- **In-engine cutscenes twice as fast at 60 fps** (issues #1 and #2): a cutscene's clock counted one 30 fps frame
  per game update, so at 60 fps the actors, the camera and the subtitles ran twice as fast. The clock now steps
  30 frames per second at any frame rate (patch set `event`, `ACR_PATCH_EVENT=0` turns it off); checked on the
  opening cutscene of the second story battle, shot for shot against 30 fps. Cutscenes still animate at 30 frames a
  second, as on the Switch.
- **The whole game too fast at 120, 144 and 240 fps** when the frame rate stays under the limit (issue #1): the game
  updates at the pace of the limit on its own thread, so it made more updates than frames shown (240 set, 120 shown:
  about 230 updates a second), and each update stepped a whole frame's time: the game ran up to twice as fast. Each
  update now steps the time shown divided by the updates made in it; measured speed 1.00 at 120, 144 and 240 (it was
  1.91 at 240), and the cutscene above keeps the 30 fps timing at 240.
- **Render scale 125 % and 140 % are back** (supersampling, issue of 0.1.0): the game keeps its GPU textures in a pool
  of 1510 MiB, cut from a GPU heap of fixed size, and the larger scene filled it, so a story cutscene of the first
  battle loaded forever. Both now grow with the scene (patch set `texpool`, `ACR_PATCH_TEXPOOL=0` turns it off): 1786
  MiB at 140 %.
- **Presentation on graphics cards without Mailbox** (AMD on Windows, issue #4): the game fell back to plain VSync,
  which holds a late frame for a whole refresh, so a GPU-bound 50 fps judders between 60 and 30. It now takes
  Adaptive VSync (FIFO relaxed) where the driver has it: frames on time wait for the refresh as before, a late one is
  shown at once. The menu shows "in use: Adaptive VSync", and the log lists the present modes the card offers.
- **GPU busy in the performance overlay** now counts the copies made to the graphics card before each part of a frame
  (VRAM mirror). They were counted as idle time, so on an RX 6800 running at 99 % the overlay showed 3 to 5 ms of GPU
  work a frame (issue #4). At 1080p on an RTX 4070 the same scene reads 8.1 ms instead of 3.3 ms.
- **Less GPU time a frame**: the game flushes about a dozen small ranges of its GPU buffer pool each frame (about
  9 MiB at 1080p), and the runtime copied the whole span between the first and the last of them to the graphics card
  every frame: about 115 MiB at 1080p, 3 ms of an RTX 4070 (11 ms with a 3840x2160 scene). Only the flushed ranges
  are copied now, 20 MiB a frame at 1080p. This weighs most where copies to the card are slow, such as the AMD cards
  of issue #4 (GPU at 99 % for little power).
- **Unlit picture with a black rectangle after "New game" at render scales above 100 %**: while a new game loads, the
  game copies the last picture into a texture of the scene's size, with a rectangle of the output's size. A larger
  scene got the picture in its top left corner only, and the rest still held the colours of the last scene, shown
  unlit at 150 % and 200 %. The copy now covers the whole texture (patch set `hold`, `ACR_PATCH_HOLD=0` turns it off);
  checked at 125, 150 and 200 % on the same recorded inputs: the screen stays black while loading, as at 100 %.
- **Long freeze when the first mission starts, on a first launch**: the setup translates each of the game's shaders
  once, answering its questions about what it reads with defaults: a 2D texture at each slot, float vertex inputs.
  Play binds something else to some of them: buffers where a shader fetches the light and decal lists, integer bone
  and instance indices, 3D or cube textures, images. These variants were translated while the frame waited: on an
  RTX 4070 the first frame of the tutorial battle took 2.6 s (2 s of it translation), and two more 0.8 and 0.9 s.
  The setup now reads how each translated shader uses its textures and vertex inputs, and translates the variants
  play asks for as well, and leaves out the default variant of a fragment shader that reads such a buffer (play never
  used one). On the same recorded inputs from a new game, that first frame takes 0.5 to 0.6 s (the graphics driver
  building the scene's pipelines, once) and nothing is translated during the 5 minutes (100 shaders before). The setup's
  shader step takes about 3 minutes more (8.4 instead of 5.3 on that PC) and the shader cache 1.1 GB more (7.6 GB).
  An install prepared by an earlier version prepares again at its next update, translating only the new variants.

### Added
- **Render scale 150 %, 175 % and 200 %** (up to 3840x2160 for a 1920x1080 picture): the larger scenes that stopped
  the game as the second intro movie started were the same full texture pool, and from 175 % on the game's pool of
  GPU buffers (215 MiB, cut from the same heap) ran out when a save was loaded. It now grows with the scene too, by
  116 MiB at 200 %.
- **Game resolution 2560x1440 and 3840x2160** (Screen tab): the game draws its picture and its HUD at that size, so
  text, icons and the minimap stay sharp on a 1440p or 4K screen. The game made its window's images a fixed 1920x1080,
  so a larger picture showed only its top left corner; they now take the game resolution. The 3D scene keeps at most
  3840x2160 pixels: at 3840x2160, 100 % draws it at full size, and Game (75 %) with DLSS or the game's FSR brings a
  2880x1620 scene up to 4K for less GPU work. DLSS also runs at these sizes (the game's scaling draw there is in its
  1920x1080 layout units). At these resolutions 100 % draws the scene 2 pixels smaller on each side (2558x1438 at
  2560x1440): a scene of exactly the output's size had the passes of in-engine cutscenes drawn in its top left corner,
  and a cutscene showed only the top left 56 % of its picture, enlarged.
- **Two-player co-op with a second controller** (issue #1): the game saw a single controller, so "Two players" on the
  map found no one to join. A second connected controller is now player 2 (the game's controller 2, a full
  controller): the map's two-player prompt accepts it, the battle splits the screen and each controller drives and
  rumbles for its own player. If player 1's controller is unplugged, player 2's takes its place. `ACR_PLAYER2=0`
  keeps the second controller out of the game.
- **Update without the NSPs**: when the install folder already holds the game, the setup empties and greys out the
  NSP folder and only offers **Update**, which builds from the game data the first build extracted
  (`make_build.py --update`). The DLC unpacked then are kept; to add one, build from the NSP folder.

## 0.1.1

### Fixed
- **Endless loading at the first cutscene** with the 3D render scale at 125 % or 140 %: the game's GPU texture pool
  has a fixed size and the larger scene filled it. Render scale now goes up to 100 %; a `settings.ini` with 125 or
  140 is read as 100. Supersampling comes back once that pool can be grown.

## 0.1.0 (beta)

First beta of AgeOfCalamityRecomp: Hyrule Warriors: Age of Calamity (update 1.3.0) built for Windows and the Steam
Deck from your own copy of the game.

### Setup
- The Windows download is an installer: the zip holds only the setup, which asks where to put the game
  (`%LOCALAPPDATA%\AgeOfCalamityRecomp` by default) and builds it there. The extracted zip can be deleted afterwards;
  a newer zip installs over the same folder.
- A plain setup window, started by `AgeOfCalamityRecomp Setup.exe` on Windows and `setup.sh` on Linux: the folder with
  your NSPs and `prod.keys` with a check mark for each file found, the platforms, a progress bar in percent and the
  log behind **Show details**. Where Smart App Control refuses the setup exe, `python/Setup (if blocked).exe` opens the
  same window.
- It is an installer only: the game's settings are in the game (F1, or Minus + Plus on a controller). Run it again on
  a folder that already holds the game and **Update** installs the new version over it, redoing only what changed and
  keeping your saves and settings.
- The Windows build uses the whole PC: a first install takes about 20 minutes on an 8-core PC with an NVIDIA card,
  the shader preparation included, and the progress bar follows it to the end. A dump on another disk is read in
  place instead of being copied first.
- **Shaders prepared before play**: every one of the game's 40,727 shaders is translated by the setup (and, on AMD,
  built into the graphics driver's cache), so a fight does not stop to translate them. Without it, the first seconds
  of each fight skipped about 170 draws (objects missing, effects blinking) and stalled for up to 700 ms.
- With an NVIDIA RTX graphics card the setup downloads NVIDIA's DLSS library (`nvngx_dlss.dll` 310.9.1, from NVIDIA's
  DLSS SDK repository, checked against its pinned hash).
- The game goes in the Start Menu, and on the desktop unless the box is unticked, with its own icon.
- The Steam Deck part says how the folder reaches the Deck (copied by hand or sent over SSH) and when the 10.9 GB of
  game data are needed.
- A `recomplauncher.json` for RecompLauncher: the build, the game, its saves, logs and main settings.

### Graphics
- **60 fps**: Frame rate 60 in the quick menu or the full menu. The game runs at the right speed (the same distance
  run and the same dialogue timing as at 30), and a 1080p battle holds 59-60 fps on a Ryzen 7 5800X with an RTX 4070.
  Big fights hold it too: the frames are now prepared for the GPU on a thread of their own (Advanced tab, GPU frame
  prep on its own thread, on by default), where the game thread alone fell to about 45 fps.
- **NVIDIA DLAA** (DLSS at full resolution) works in Age of Calamity: it finds the game's own scene depth and camera,
  gets the camera's motion with a reprojection worked out in double precision, uses NVIDIA's transformer model
  (preset K), the real time between frames and a jitter sequence as long as NVIDIA asks for. Edges are smooth and
  details stay steady when the camera moves.
- **DLSS Quality, Balanced and Performance** now really run DLSS: the scene is drawn at a lower resolution and
  DLSS brings it up to the output, in place of the game's own scaling. The HUD stays at full resolution and is
  never passed through DLSS.
- **DLSS model** setting (Screen tab): Automatic (NVIDIA's advice per mode, K for DLAA), K, or J, which leaves a
  little less ghosting behind moving characters for a little more flicker. It applies at once.
- **Steadier 60 fps**: the shader cache is saved in calm moments (5 s without a new shader, at most every 2 minutes,
  and at exit) instead of every 20 s, which dropped 60 fps to 54 for a second every 24 s in long battles.
- **120, 144 and 240 fps (experimental, still in development)** in the Frame rate setting (full menu and quick menu), applied at once in the middle of a
  game: as in the TotK recomp, above 60 the game steps by the real time of each frame, so it runs at the right speed
  at any rate, cutscenes included. Smoother battles too: about half the time of turning the game's graphics commands
  into Vulkan went to a check made on every draw, and the game's main thread no longer waits as often for its
  processor core.
- **FXAA** for every graphics card: the picture goes through an FXAA pass just before the game draws the HUD, so the
  edges of the 3D scene are smoothed while the HUD stays sharp. It is on by default and
  turns itself off while DLAA or DLSS runs.
- **Shadow resolution** works in Age of Calamity: the sun's 1024x2048 shadow map is drawn up to 4 times larger on
  each side, so shadow edges on stairs and walls no longer show steps that crawl when the camera moves.
- **Render scale** up to 140 %: 100 % draws the 3D scene at full 1920x1080, and 125 % and 140 % draw it larger and
  scale it down (supersampling, 2688x1512 at 140 %). 150 % and more crash the game: one of its own fixed memory heaps
  runs out when a cutscene starts.
- With fullscreen on, the game opens fullscreen straight away, behind your other windows while it loads, instead of
  showing its picture in the top left corner and then growing to the whole screen.
- The options menu (F1) has its own Sheikah-blue look and shows only the settings that do something in this game.
- **Frame queue** (Game tab, off by default): the game starts its next frame while the previous one waits for its
  turn on screen, so a slow frame is less likely to be dropped, for up to one frame more input delay.

### Sound
- **Output device** (Sound tab): the system default, or a device chosen by name. The sound moves to it at once; if it
  is unplugged the system default plays until it is back.

### Controls
- **Xbox button prompts** (Controls tab, Button prompts: Xbox; applies at once, no restart): the game's prompts and menus show A, B,
  X and Y in the Xbox colours, LB, RB, LT, RT, Menu and View. With the Nintendo layout (A on the right button) the
  prompt for A shows B, as that button is labelled B on an Xbox pad. The setup draws them over the game's own button
  textures from your copy of the game.
- **PlayStation button prompts** (Button prompts: PlayStation): cross, circle, square and triangle in their colours,
  L1, R1, L2, R2, Options and Create.
- **One button layout**: every controller plays by position as on the Switch (A on the right button: circle on a
  PlayStation pad, B on an Xbox pad). The Button layout option is gone (an older settings.ini reads as Nintendo); the
  remapping rows of the Controls tab change any button instead.
- Keyboard and mouse with every key rebindable; Xbox, PlayStation and Switch controllers with rumble and motion.

### Cleanup
- Settings left over from the recompilations this port started from, which never did anything in Age of Calamity,
  are gone: shadow map size, shadow cascade range, field of view, stick camera speed, game speed and cloth on the
  clock, and high frame rate fixes. Lines left for them in `settings.ini` do nothing (the log names them unknown).
- Code made for those games' files and versions is gone too (their resource size table for mods, their zstd
  decoder, a wait made for one Astral Chain version's uniform buffers): Age of Calamity has none of them.
- Earlier logs are kept as `logs/game.1.log` to `game.4.log`. On Linux the window belongs to
  `AgeOfCalamityRecomp.desktop`, so the taskbar shows the game's icon.

### Known limits
- Above 60 fps a busy battle stays around 85-110 fps on a Ryzen 7 5800X: the game's main thread is the limit.
- With Xbox or PlayStation prompts, the L and R tabs at the top of the menus and the stick and d-pad icons keep the Switch's look.
- In DLSS Quality, Balanced and Performance, each step of the game's own dynamic resolution (it lowers the 3D
  resolution down to 70 % in big fights, then raises it again a step at a time) starts DLSS over, which costs
  15 to 35 ms (sometimes a dropped frame) and a frame of softer edges.
- The first seconds after a fight starts are a little soft: the game loads its sharp textures progressively, as on
  the Switch.
