Parallel Belief Propagation Image Super-resolution

Nathaniel Bowman, Erin Carrier, Ralf Gunter, Anirudh Jayakumar

University of Illinois, Urbana-Champaign

This project implements parallel belief propagation for image super-resolution using Charm++. In
general, image super-resolution is an algorithm designed to improve the resolution of images. It takes
a small, pixel-based images and attempts to create a larger (resolution) image. Standard algorithms,
such as cubic-spline upsampling produce blurry images. This project implements a specific image superresolution
algorithm designed by Freeman et al. [1] that attempts to improve on standard algorithms
for upsampling in parallel using Charm++.

[1] W.T. Freeman, T.R. Jones, and E.C. Pasztor. Example-based super-resolution. Computer Graphics and Applications, 
IEEE, 22(2):56–65, Mar 2002. ISSN 0272-1716.
