## Tiny Space

A small space sim intended as a base for testing features like real-time saves.
There are no controls. It's just a self-perpetuating sim. Quit with <Ctrl+C>.

Ships fly around, hopping sectors. Speed depends on ship class.
I'll probably add some simple combat, jump gates, stations, tasks, and basic path finding as time permits.

Displays in-sector ship information, a sector map, and a global map showing how many ships are in each sector.

This gist will remain a simple base, but will be updated with more features occasionally to increase complexity.
Separate gists will be created for feature tests.

## Sample Output
```
> ( name:F001 code:AEI-563 loc:{-1.4, 1.7} dir:{-0.4,-0.9} class:Frigate   )
  ( name:C031 code:YHV-953 loc:{ 0.3, 1.3} dir:{ 0.3, 1.0} class:Corvette  )
  ( name:S059 code:RKC-685 loc:{-3.6, 3.3} dir:{-1.0, 0.1} class:Scout     )
  ( name:S062 code:CTW-818 loc:{ 4.4, 4.2} dir:{-1.0,-0.0} class:Scout     )
  ( name:F078 code:RTF-828 loc:{-1.7,-4.2} dir:{ 0.1, 1.0} class:Frigate   )
  ( name:F119 code:YZC-396 loc:{ 2.6,-1.9} dir:{ 0.3,-1.0} class:Frigate   )

                   +-[ 08D ]-------------------------+
                   |                                 |
                   |          .                      |
                   |                                 |
                   |                         .       |
                   |                                 |
                   |                                 |
                   |                .                |
                   |             ^                   |
                   |    .                            |
                   |                            .    |
                   |                                 |
                   +---------------------------------+

        A      B      C      D      E      F      G      H      I      J  
   .----------------------------------------------------------------------
01 |    3      5      7      7      7      4      3      6      5      5  
02 |    6      5      7      7      7      1      4      5      1      4  
03 |    7      3      2     11      5             6      2      5      4  
04 |    2      5     10      3      3      8      2      3      3      8  
05 |    2      3      5      7      7      8      7      8      7      7  
06 |    4      4      4      6      3      5      5      3      7      5  
07 |    6      2      3      5      3      1      8      5      3      2  
08 |    4      5      7  [   6 ]    3      4      5      8      3      6  
09 |    5      4      7      3      6      6      5      3      6      6  
10 |    9      7      2      5      6     10      6      7      6      4  

work: 0.084438ms  display: 0.598263ms
```
