/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _lua_class_register_h_
#define _lua_class_register_h_

#include "plmscript_config.h"
#include "plm_path.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "lua_class_image.h"
#include "lua_class_structs.h"

#define _MAX_STAGES 20

class Registration_parms;

typedef struct lua_register_struct lua_register;
struct lua_register_struct {
    char fn[_MAX_PATH];
    Registration_parms *regp;
    lua_image *moving;    
    lua_image *fixed;    
    lua_ss *moving_mask;
    lua_ss *fixed_mask;
};


PLMSCRIPT_C_API int register_lua_class_register (lua_State *L);
PLMSCRIPT_C_API void init_register_instance (lua_register* lregister);

#endif
