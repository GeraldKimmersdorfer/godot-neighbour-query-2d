## Benchmark 04.01 DEBUG
**Time:** 15:21:10
**CPU:** Intel(R) Core(TM) i5-10600 CPU @ 3.30GHz
**Common:** time=20s, speed=3.00 rad/s
| metric | 5000 DENSITY NONE | 500 DENSITY NONE | 0 DENSITY NONE | 5000 UNIFORM NONE | 5000 UNIFORM STRAIGHT |
| --- | --- | --- | --- | --- | --- |
| get_closest | 9.38 us | 6.39 us | 2.89 us | 10.44 us | 10.13 us |
| get_all | 30.38 us | 8.46 us | 2.78 us | 35.26 us | 34.10 us |
| get_next | 4.85 us | 3.09 us | 1.66 us | 5.64 us | 5.39 us |
| get_random | 11.01 us | 6.56 us | 1.25 us | 13.80 us | 11.99 us |
| get_next_first | 0.92 us | 1.21 us | 1.07 us | 0.84 us | 0.80 us |
| refresh | 1.50 ms | 0.19 ms | 1.87 us | 1.59 ms | 1.44 ms |

## Benchmark 04.01 RELEASE
**Time:** 15:23:23
**CPU:** Intel(R) Core(TM) i5-10600 CPU @ 3.30GHz
**Common:** time=20s, speed=3.00 rad/s
| metric | 5000 DENSITY NONE | 500 DENSITY NONE | 0 DENSITY NONE | 5000 UNIFORM NONE | 5000 UNIFORM STRAIGHT |
| --- | --- | --- | --- | --- | --- |
| get_closest | 8.37 us | 6.40 us | 2.97 us | 9.46 us | 9.55 us |
| get_all | 25.20 us | 7.71 us | 2.52 us | 28.17 us | 29.04 us |
| get_next | 4.53 us | 3.01 us | 1.57 us | 5.32 us | 5.32 us |
| get_random | 10.61 us | 6.44 us | 1.32 us | 12.60 us | 12.16 us |
| get_next_first | 0.84 us | 1.15 us | 1.14 us | 0.75 us | 0.75 us |
| refresh | 1.10 ms | 0.14 ms | 1.63 us | 1.10 ms | 1.05 ms |