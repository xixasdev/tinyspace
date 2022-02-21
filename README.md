## Tiny Space

A small terminal-based space sim, originally intended as a base for testing features like real-time saves.
There are no controls. It's just a self-perpetuating sim. Quit with `Ctrl+C`.

Ships fly around, hopping sector-to-sector. Speed depends on ship class.
Displays in-sector ship information, a sector map, and a global map showing how many ships are in each sector.

This gist will remain a simple base, but may be updated with more features occasionally to increase complexity.
Separate gists will be created for versions and/or feature tests.

**Build and Run:**
```
$ cd /path/to/tinyspace
$ make
$ ./tinyspace --color
```

**Options:**
- `--color` - Enable color display (for terminals that support ANSI color codes)
- `--no-jumpgates` - Disable jumpgate travel and revert to the original fly-between-sectors style.

**Note:**
The `--no-jumpgates` option is currently broken, as ships will now always seek a destination.
Without jumpgates, that destination will always be a station or random location.
As such, they will never leave their sector.
This was an oversight and should be fixed in v4.

## What's New

**New in this version (v3 - snapshot serialization):**
- Data snapshots and background saves
- Separated single-file example into multiple files

**Details:**
- Runs for 10 seconds.
- Takes a snapshot of live game data.
- Serializes live game data to `tinyspace_01-live.txt` (on main thread).
- Spawns a background thread that waits 5 seconds then serializes snapshot to `tinyspace_02-snap.txt` (while main thread continues)
- Continues simulation for 10 seconds.
- Serializes live game data to `tinyspace_03-live.txt` (on main thread).
- Spawns a background thread that waits 5 seconds then serializes (original) snapshot to `tinyspace_04-snap.txt` (while main thread continues)
- Exits.

## Implementation

Uses a `Saveable` class similar to previous gists I've written on the subject (favoring pointer style this time around), but with cleaner type erasure.
The the non-specific `Saveable` operator overloads have their return values keyed on current thread ID.
So when `isSavingThread` returns `true`, during a non-specific `Saveable` access, the return value is from the active snapshot.
Basically, it creates a thread-based transaction view. Not the most efficient way to do it, but the quickest and cleanest method for this sample.

4 save files are written to disk -- an actual copy of live followed by a copy from the snapshot, this happens at both 10 and 20 seconds of the run.
A 5-second delay has been applied before background serialization begins to ensure a significant delta between live and snapshot data.
Unlike a standard save case, the `isSaving` flag is not cleared after the first snapshot, and the snapshot is instead held open through the second save set (but the save thread is joined back to main after serialization).

So there are 4 save files -- 3 of which (1, 2, and 4) should be exactly the same (excluding their "LIVE" or "SNAP" header and trailing timestamp).

This allows us:
- to verify the snapshot perfectly matches the live data
- to see what's changed over a 10 second period
- to see that the snapshot is still exactly the same on a new save thread 10 seconds later

Serialization's baked into the model classes.
I usually prefer to keep serialization code separate when not using something like Protobuf, but it made for easier reading in a single-file example.

**Note:**
It took a little bit to write up the templates for proper type erasure -- but no more than an afternoon.
The extra work's mainly due to inline use case differences between standard vars, pointers, and shared_ptr types.
Short of it is that the Saveable template requires two template specializations to provide reasonable erasure for pointer types.

## Required Source Modifications

Any model values that change after initialization were replaced, but only at their point of declaration -- didn't even have to change constructor signatures. Type erasure takes care of type conversion for normal use in the rest of the application.

The most common required change was having to add a top-level `()` or `->` to access nested properties of saveable types.
This is pretty standard. It's the equivalent of having to call `.get()` or use `->` when converting a stack value or raw pointer type to `shared_ptr`.

The `Ship` struct needed a new copy constructor, but that's beyond minor.

I extended `Vector2` to `SaveableVector2`.
This wasn't strictly necessary, but it made for much cleaner operator overloads on the inline math functions and it's hidden away behind full type erasure.

The biggest required change was taking a functional programming approach to model-owned relational collections, ensuring they are always replaced and not modified. While this sounds like a heavy change-up, it's generally lighter than synchronizing on anything but very large collections requiring huge allocations. In such cases, allocations should be prepared on a background thread to avoid killing your render cycle -- but that's outside the scope of this example.

## Sample Simulation

![Screenshot](screenshot.png)

## ...or plain Black and White
```
 > HKY-490 ||||| [-1,-5] NE Scout    
 + WHL-507 ||||| [ 6, 2]  E Scout    
 + UKZ-959 ||||| [-1, 2] SW Frigate   -> Frigate   WDE-501 |||||
   UNW-919 ||||| [-1, 4] S  Frigate  
 + OWC-810 ||||| [-0, 7] NW Transport
   ZHR-170 ||||| [-3,-5] SW Transport
 - WDE-501 ||||| [-4, 4] NE Frigate   -> Frigate   UKZ-959 |||||

      +-------[ SAVE IN PROGRESS... BUT GAME'S STILL RUNNING! ]-------+
      |                                                               |
      |                                                               |
      |                         0                                     |
      |                               .                               |
      |                                                               |
      |                                                               |
      |                   .        .                                  |
      |                                                               |
      |                            .                    0             |
      |                                                               |
      |                                                               |
      |                                                               |
      |                                                               |
      |                                                               |
      |                                                               |
      |                      .  () >                                  |
      |                                                               |
      |          ()                                                   |
      |                                                               |
      |                                                               |
      |                                                               |
      +-[ H09 ]-------------------------------------------------------+

        A      B      C      D      E      F      G      H      I      J  
   .----------------------------------------------------------------------
01 |    2      6      2      8      6      1      8      8      4      7  
02 |   10      2      5      7      6      8      5      3      1      6  
03 |    5      7      5      5      5      5      3      4      8      5  
04 |    8      1      3      8.     3      7      5      4      5      5  
05 |    1      3      4      7      5     11      4      3      6      9. 
06 |    6      2      2      6      9      4      1      3      9      3  
07 |    6      9      4      9     12.     4      5      4      2      5  
08 |    4      4      4      6      5      4      6      4      6      3  
09 |    8      3      4      5      5      3         [   7 ]    8      2  
10 |    6      4      3      6      5      4      1      5      3      8  

delta: 300.106ms  work: 2.17165ms  display: 1.23361ms
```
