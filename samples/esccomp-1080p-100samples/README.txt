1080p image rendered at 100 samples (~45 minutes to render, < 10 seconds to filter)

Command Line:
=============
./rpnr -e 0001.png -fradius 15

Using the following inputs:
===========================
combined
ao
diffusecolor
normal
z
emission

Parameter values:
=================
Output radius (-fradius): 15
Noisy delimiter radius (-fbnoisy): 5
Ambient Occlusion delimiter radius (-fbao): 7
Minimum delimiter noise tolerance (-tolmin): 16
Maximum delimiter noise tolerance (-tolmax): 28
Z delimiter surface tolerance (-tolz): 4
Diffuse delimiter tolerance (-toldiffuse): 4
Normal delimiter tolerance (-tolnormal): 16
Glossy delimiters tolerance (-tolglossy): 96
Translucent delimiter tolerance (-toltranslucent): 96
Edge blur weight (-edgeblurweight): 50
