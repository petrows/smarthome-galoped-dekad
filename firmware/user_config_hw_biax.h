/*
  Part for Galoped-dekad hw version:

    * Drive: Bi-Axial
    * Backlight: RGB
*/

#undef FRIENDLY_NAME
#define FRIENDLY_NAME "Galoped-dekad-biax"

#define VID6608_STEPS_1 12 * 275
#define VID6608_STEPS_2 12 * 320

#define USER_TEMPLATE "{\"NAME\":\"Galoped-dekad-biax\",\"GPIO\":[1,1,1,1,1,1,1,1,1,1,0,1376,1600,1632,1,1,0,640,608,1,0,1,12160,12192,0,0,0,0,12161,12193,32,1,1,0,0,1],\"FLAG\":0,\"BASE\":1}"

#undef VID6608_RESET_ON_INIT
#define VID6608_RESET_ON_INIT true
