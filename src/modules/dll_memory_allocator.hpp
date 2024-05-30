// Basically, if you allocate memory in one exe/dll and then free it in another, you segfault.
// Each dll has all the GE/SAS/etc. functions compiled into it so , and it would be a lot of work to have wrapper functions
//   for every single engine/libray function a dll might want to call (although honestly that would be smart because it's a dumb idea to have a copy of every function compiled into every file)
// DISREGARD LOL

#ifdef IS_MODULE

#else

#endif