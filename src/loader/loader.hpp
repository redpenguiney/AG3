// The way AG3 Engine works is this:
// an .agtgame file is written to by the AG3 editor, which is from a technical viewpoint like any game run using the AG3 Engine.
// the AG3.exe takes one command line argument, a path to a .agtgame file which describes all the gameobjects + assets + dlls/scripts + etc to load at runtime.
// This loader function parses that file and returns a list of all the assets/gameobjects/scripts/etc that file requests.
// Once the engine makes all that, the game actually runs. 
// The editor also uses this function so it can load .agtgame files while it's running (so you can actually, yunno, edit) 
// ---
//
// Takes a path to the .agtgame file to load.
void LoadGame(const char* filepath);
    