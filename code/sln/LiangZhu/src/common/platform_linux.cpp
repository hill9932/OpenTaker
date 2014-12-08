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
    ON_ERROR_LOG_LAST_ERROR_AND_DO(pw, ==, NULL, return err);

    // Drop privileges
    if ((setgid(pw->pw_gid) != 0) || (setuid(pw->pw_uid) != 0)) 
    {
        int err = LiangZhu::GetLastSysError();
        LOG_ERROR_MSG(err);
        return err;
    } 


    RM_LOG_INFO_S("Success to change user to " << username);

    umask(0);
    return 0;
}

#endif