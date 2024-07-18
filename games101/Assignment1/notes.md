## Key notes

### 1, how to get the rotation matrix around z-axis:

First we know that the z-coordinates will not change after rotation, second, just image the whole proces, if we pick a random point and then connect the origin and the point and that point after rotation, we will get a triangle, while the top angle is theta. Then image we punch the triagngle to the xy plane, the angle doesn't change, which means for the x and y coordinates, they are just the projection of the original point on the xy plane. So we can get the rotation matrix around z-axis.

### 2, how to get the projection matrix: just image two planes and get the coordinates by similar triangles.

### 3, how to get the rotation matrix around an arbitrary axis: 1. rotate that axis to z, 2. rotate theta around z-axis 3. rotate back. 

