/*   $OSSEC, run_rk_check.c, v0.1, 2005/09/30, Daniel B. Cid$   */

/* Copyright (C) 2005 Daniel B. Cid <dcid@ossec.net>
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

  
#include <stdio.h>       
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>
#include <errno.h>


#include "headers/defs.h"
#include "headers/debug_op.h"

#ifdef OSSECHIDS
#include "headers/mq_op.h"
#endif

#include "rootcheck.h"

#include "error_messages/error_messages.h"


/* notify_rk
 * Report a problem.
 */
int notify_rk(int rk_type, char *msg)
{
    /* Non-queue notification */
    if(rootcheck.notify != QUEUE)
    {
        if(rk_type == ALERT_OK)
            printf("[OK]: %s\n", msg);
        else if(rk_type == ALERT_SYSTEM_ERROR)
            printf("[ERR]: %s\n", msg);
        else
        {
            printf("[FAILED]: %s\n", msg);
        }

        printf("\n");
        return(0);
    }
   
    /* No need to alert on that to the server */
    if(rk_type <= ALERT_SYSTEM_ERROR)
        return(0);

    #ifdef OSSECHIDS    
    if(SendMSG(rootcheck.queue, msg, ROOTCHECK, ROOTCHECK_MQ) < 0)
    {
        merror(QUEUE_SEND, ARGV0);

        if((rootcheck.queue = StartMQ(DEFAULTQPATH,WRITE)) < 0)
        {
            ErrorExit(QUEUE_FATAL, ARGV0, DEFAULTQPATH);
        }

        if(SendMSG(rootcheck.queue,msg,ROOTCHECK,ROOTCHECK_MQ) < 0)
        {
            ErrorExit(QUEUE_FATAL, ARGV0, DEFAULTQPATH);
        }
    }
    #endif

    return(0);        
}

 
/* start_rk_daemon
 * Start the rootkit daemon variables
 */
void start_rk_daemon()
{
    return;
        
    if(rootcheck.notify == QUEUE)
    {
    }
}


/* run_rk_check: v0.1
 * Execute the rootkit checks
 */
void run_rk_check()
{
    int i;

    FILE *fp;
   
    /* Hard coding basedir */ 
    char basedir[] = "/";

    /* Removing the last / from basedir */
    i = strlen(basedir);
    if(i > 0)
    {
        if(basedir[i-1] == '/')
        {
            basedir[i-1] = '\0';
        }
    }
  
    /*** Initial message ***/
    if(rootcheck.notify != QUEUE)
    {
        printf("\n");
        printf("Starting rootcheck (http://www.ossec.net/rootcheck)\n");
        printf("Be patient, it may take a few minutes to complete...\n");
        printf("\n");
    }
 
    /* Cleaning the global variables */
    rk_sys_count = 0;
    rk_sys_file[rk_sys_count] = NULL;
    rk_sys_name[rk_sys_count] = NULL;


    /***  First check, look for rootkits ***/
    /* Open rootkit_files and pass the pointer to check_rc_files */
    if(!rootcheck.rootkit_files)
    {
        merror("%s: No rootcheck_files file configured.", ARGV0);
    }

    else
    {
        fp = fopen(rootcheck.rootkit_files, "r");
        if(!fp)
        {
            merror("%s: No rootcheck_files file: '%s'",ARGV0, 
                    rootcheck.rootkit_files);
        }

        else
        {
            check_rc_files(basedir, fp);

            fclose(fp);
        }
    }
  
  
    /*** Second check. look for trojan entries in common binaries ***/
    if(!rootcheck.rootkit_trojans)
    {
        merror("%s: No rootcheck_trojans file configured.", ARGV0);
    }
    
    else
    {
        fp = fopen(rootcheck.rootkit_trojans, "r");
        if(!fp)
        {
            merror("%s: No rootcheck_trojans file: '%s'",ARGV0,
                                        rootcheck.rootkit_trojans);
        }

        else
        {
            check_rc_trojans(basedir, fp);

            fclose(fp);
        }
    }
   
    /*** Third check, looking for files on the /dev ***/
    check_rc_dev(basedir);
    
    /*** Fourth check,  scan the whole system looking for additional issues */
    check_rc_sys(basedir);
    
    /*** Process checking ***/
    check_rc_pids();         

    /*** Check all the ports ***/
    check_rc_ports();    

    /*** Check open ports ***/
    check_open_ports();
        
    /*** Check interfaces ***/
    check_rc_if();    
   
   
    /* Cleaning the global memory */
    {
        int li;
        for(li = 0;li <= rk_sys_count; li++)
        {
            if(!rk_sys_file[li] ||
               !rk_sys_name[li])
                break; 

            free(rk_sys_file[li]);
            free(rk_sys_name[li]);
        }
    }

    debug1("%s: DEBUG: Leaving run_rk_check",ARGV0); 
    return;
}


/* EOF */
