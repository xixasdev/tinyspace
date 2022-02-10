## Tiny Space

A small terminal-based space sim, originally intended as a base for testing features like real-time saves.
There are no controls. It's just a self-perpetuating sim. Quit with `Ctrl+C`.

Ships fly around, hopping sector-to-sector. Speed depends on ship class.
Displays in-sector ship information, a sector map, and a global map showing how many ships are in each sector.

This gist will remain a simple base, but may be updated with more features occasionally to increase complexity.
Separate gists will be created for versions and/or feature tests.

**Options:**
- `--color` - Enable color display (for terminals that support ANSI color codes)
- `--no-jumpgates` - Disable jumpgate travel and revert to the original fly-between-sectors style.

## What's New

**New in this version (v2):**
- Added a full color interface
- Friend/foe designations ( `+` / `-` ) and coloring
- Jumpgate travel between sectors

**New Options:**
- `--color` - Enable color display (for terminals that support ANSI color codes)
- `--no-jumpgates` - Disable jumpgate travel and revert to the original fly-between-sectors style.

## Sample Simulation

![Screenshot](screenshot.png)

## ...or plain Black and White
```
 >( name:F001 code:PTS-016 loc:{-6.4, 1.9} dir:{ 0.5,-0.9} class:Frigate   )
  ( name:T010 code:PXO-248 loc:{-0.4,-4.1} dir:{ 0.1,-1.0} class:Transport )
  ( name:F017 code:VIH-960 loc:{-1.8,-8.9} dir:{ 0.9,-0.4} class:Frigate   )
  ( name:F025 code:SWU-004 loc:{-7.8,-5.2} dir:{ 0.5, 0.9} class:Frigate   )
  ( name:S023 code:GIF-381 loc:{-2.1, 0.4} dir:{ 0.2, 1.0} class:Scout     )
 -( name:C031 code:UCB-086 loc:{-5.4, 8.9} dir:{ 0.9,-0.5} class:Corvette  )
  ( name:C088 code:VGQ-182 loc:{-0.8,-8.0} dir:{ 0.8, 0.6} class:Corvette  )
  ( name:C113 code:AWF-748 loc:{ 0.2,-5.5} dir:{ 0.9, 0.5} class:Corvette  )
  ( name:S100 code:JMK-282 loc:{ 8.7,-2.8} dir:{ 0.4, 0.9} class:Scout     )

      +---------------------------------------------------------------+
      |                               0                               |
      |                .                                              |
      |                                                               |
      |                                                               |
      |                                                               |
      |                                                               |
      |                                                               |
      |                                                               |
      |             v                                                 |
      |                                                               |
      |                         .                                   0 |
      |                                                               |
      |                                                               |
      |                                                          .    |
      |                               .                               |
      |       .                                                       |
      |                               .                               |
      |                                                               |
      |                            .                                  |
      |                         .                                     |
      |                               0                               |
      +-[ A07 ]-------------------------------------------------------+

        A      B      C      D      E      F      G      H      I      J  
   .----------------------------------------------------------------------
01 |    6      5      5      3      4      4      9      5      4      6  
02 |   12     10      5      9      3      8      9      3      4      7  
03 |    5      3      6     10      5      5      7      4      4      9  
04 |    6      6      4      9      2      3      8      4      4      1  
05 |    6      7      1      5      2      2      5      3      6      5  
06 |    8      3      2      2      7             3      4      4      8  
07 |[   9 ]    2      4      4      1      7      6      3      4      3  
08 |    5      6      3      8      3      2      4      2      3      3  
09 |    7      5      5      5      7      4      3      4      7      7  
10 |    6      6      8      4      4      5     10      3      3      6  

delta: 300.069ms  work: 0.037854ms  display: 0.280972ms
```
