#include "common.h"
#include "platform.h"

#ifdef LINUX

#include <pwd.h>


int DropPrivileges(const char* _username) 
{
    const char* username = _username ? _username : "anonymous";
    struct passwd *pw = NULL;

    if (getgid() && getuid()) 
    {
        RM_LOG_DEBUG("I'm not a superuser.");
        return 0;
    }

    pw = getpwnam(username);
    ON_ERROR_PRINT_LASTMSG_AND_RETURN(pw, ==, NULL);

    // Drop privileges
    if ((setgid(pw->pw_gid) != 0) || (setuid(pw->pw_uid) != 0)) 
    {
        int err = GetLastSysError();
        LOG_ERRORMSG(err);
        return err;
    } 


    RM_LOG_INFO_S("Success to change user to " << username);

    umask(0);
    return 0;
}

#endif