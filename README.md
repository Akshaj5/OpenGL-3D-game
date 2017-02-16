### Abstract
A 3-D game where we have to mave the block to the hole in the map.There are fragile tiles in the game on which the block cannot be vertical and you use the switches to togle the position of the bridges.Don't move out of the map otherwise block will fall down.

###Controls:

#Keyboard:
* 'Q' to quit the game
* 'UP' to move ahead
* 'DOWN' to move back
* 'RIGHT' to roll right
* 'LEFT' to roll left
* 'T' for top view.
* 'H' for tower view.
* 'B' for block view.
* 'F' for follow view.

##About the game:
* Move the block to the hole.
* Use switches to close the bridge.
* Don't be vertical on fragile tile.
* Use less moves.


##Features:

* No images used anywhere in the game. Everything is create by using shapes in openGL. This ensures the loading of the game is quick and efficient.
* Rendered text/numbers without the help of any libraries (only using shapes).
* Collision using boxes(not circles), this is a lot more effective when blocks are of uneven size.


##Note:

All objects are sorted into different layers. Each layer is drawn one at a time. Some layers are more prefered and will be drawn last  whereas others will be drawn earlier (Like the background layer). Within a layer objects are drawn in a lexicographical order. These two together give you the ability to draw complex objects with ease.

### Dependencies:
##### Linux/Windows/ Mac OSX - Dependencies: (Recommended)
* GLFW
* GLAD
* GLM

##### Linux - Dependencies: (alternative)
* FreeGLUT
* GLEW
* GLM
