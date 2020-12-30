#!/bin/sh

# Copyright (C) 2015-2020, Wazuh Inc.
# Shell script update functions for Wazuh
# Author: Daniel B. Cid <daniel.cid@gmail.com>

FALSE="false"
TRUE="true"

isUpdate()
{
    if [ $? = 0 ]; then
        if [ "X$WAZUH_HOME" = "X" ]; then
            echo "# ($FUNCNAME) ERROR: The variable DIRECTORY wasn't set" 1>&2
            echo "${FALSE}"
            return 1;
        fi
        ls -la $WAZUH_HOME > /dev/null 2>&1
        if [ $? = 0 ]; then
            echo "${TRUE}"
            return 0;
        fi
    fi
    echo "${FALSE}"
    return 1;
}

doUpdatecleanup()
{
    if [ "X$WAZUH_HOME" = "X" ]; then
        echo "# ($FUNCNAME) ERROR: The variable DIRECTORY wasn't set." 1>&2
        echo "${FALSE}"
        return 1;
    fi

    # Checking if the directory is valid.
    _dir_pattern_update="^/[-a-zA-Z0-9/\.-]{3,128}$"
    echo $WAZUH_HOME | grep -E "$_dir_pattern_update" > /dev/null 2>&1
    if [ ! $? = 0 ]; then
        echo "# ($FUNCNAME) ERROR: directory name ($WAZUH_HOME) doesn't match the pattern $_dir_pattern_update" 1>&2
        echo "${FALSE}"
        return 1;
    fi
}

getPreinstalled()
{

	echo $TYPE
    return 0;
}

getPreinstalledDir()
{
    echo "$WAZUH_HOME"
    return 0;
}

getPreinstalledVersion()
{
    echo $VERSION
}

getPreinstalledName()
{
    NAME=""
    echo $NAME
}

UpdateStartOSSEC()
{


   if [ "X$TYPE" != "Xagent" ]; then
       TYPE="manager"
   fi

   if [ `stat /proc/1/exe 2> /dev/null | grep "systemd" | wc -l` -ne 0 ]; then
       systemctl start wazuh-$TYPE
   elif [ `stat /proc/1/exe 2> /dev/null | grep "init.d" | wc -l` -ne 0 ]; then
       service wazuh-$TYPE start
   else
       $WAZUH_HOME/bin/wazuh-control start
   fi
}

UpdateStopOSSEC()
{
    if [ 1 ]
    then


        MAJOR_VERSION=`echo ${VERSION} | cut -f1 -d'.' | cut -f2 -d'v'`

        if [ "X$TYPE" != "Xagent" ]; then
            TYPE="manager"
            if [ $MAJOR_VERSION -ge 4 ]; then
              EMBEDDED_API_INSTALLED=1
            fi
        fi

        if [ `stat /proc/1/exe 2> /dev/null | grep "systemd" | wc -l` -ne 0 ]; then
            systemctl stop wazuh-$TYPE
        elif [ `stat /proc/1/exe 2> /dev/null | grep "init.d" | wc -l` -ne 0 ]; then
            service wazuh-$TYPE stop
        fi
    else
        echo "      WARN: No such file ${OSSEC_INIT}. Trying to stop Wazuh..."
        DIRECTORY=${INSTALLDIR}
    fi

    # Make sure Wazuh is stopped
    $WAZUH_HOME/bin/wazuh-control stop > /dev/null 2>&1
    sleep 2

   # We also need to remove all syscheck queue file (format changed)
    if [ "X$VERSION" = "X0.9-3" ]; then
        rm -f $WAZUH_HOME/queue/syscheck/* > /dev/null 2>&1
        rm -f $WAZUH_HOME/queue/agent-info/* > /dev/null 2>&1
    fi
    rm -rf $WAZUH_HOME/framework/* > /dev/null 2>&1
    rm $WAZUH_HOME/wodles/aws/aws > /dev/null 2>&1 # this script has been renamed
    rm $WAZUH_HOME/wodles/aws/aws.py > /dev/null 2>&1 # this script has been renamed

    # Deleting plain-text agent information if exists (it was migrated to Wazuh DB in v4.1)
    if [ -d "$WAZUH_HOME/queue/agent-info" ]; then
        rm -rf $WAZUH_HOME/queue/agent-info > /dev/null 2>&1
    fi

    # Deleting plain-text rootcheck information if exists (it was migrated to Wazuh DB in v4.1)
    if [ -d "$WAZUH_HOME/queue/rootcheck" ]; then
        rm -rf $WAZUH_HOME/queue/rootcheck > /dev/null 2>&1
    fi
}

UpdateOldVersions()
{
    if [ "$INSTYPE" = "server" ]; then
        # Delete deprecated rules & decoders
        echo "Searching for deprecated rules and decoders..."
        DEPRECATED=`cat ./src/init/wazuh/deprecated_ruleset.txt`
        for i in $DEPRECATED; do
            DEL_FILE="$INSTALLDIR/ruleset/$i"
            if [ -f ${DEL_FILE} ]; then
                echo "Deleting '${DEL_FILE}'."
                rm -f ${DEL_FILE}
            fi
        done
    fi

    # If it is Wazuh 2.0 or newer, exit
    if [ "X$USER_OLD_NAME" = "XWazuh" ]; then
        return
    fi

    OSSEC_CONF_FILE="$WAZUH_HOME/etc/ossec.conf"
    OSSEC_CONF_FILE_ORIG="$WAZUH_HOME/etc/ossec.conf.orig"

    # ossec.conf -> ossec.conf.orig
    cp -pr $OSSEC_CONF_FILE $OSSEC_CONF_FILE_ORIG

    # Delete old service
    if [ -f /etc/init.d/ossec ]; then
        rm /etc/init.d/ossec
    fi

    if [ ! "$INSTYPE" = "agent" ]; then

        # Delete old update ruleset
        if [ -d "$WAZUH_HOME/update" ]; then
            rm -rf "$WAZUH_HOME/update"
        fi

        ETC_DECODERS="$WAZUH_HOME/etc/decoders"
        ETC_RULES="$WAZUH_HOME/etc/rules"

        # Moving local_decoder
        if [ -f "$WAZUH_HOME/etc/local_decoder.xml" ]; then
            if [ -s "$WAZUH_HOME/etc/local_decoder.xml" ]; then
                mv "$WAZUH_HOME/etc/local_decoder.xml" $ETC_DECODERS
            else
                # it is empty
                rm -f "$WAZUH_HOME/etc/local_decoder.xml"
            fi
        fi

        # Moving local_rules
        if [ -f "$WAZUH_HOME/rules/local_rules.xml" ]; then
            mv "$WAZUH_HOME/rules/local_rules.xml" $ETC_RULES
        fi

        # Creating backup directory
        BACKUP_RULESET="$WAZUH_HOME/etc/backup_ruleset"
        mkdir $BACKUP_RULESET > /dev/null 2>&1
        chmod 750 $BACKUP_RULESET > /dev/null 2>&1
        chown root:ossec $BACKUP_RULESET > /dev/null 2>&1

        # Backup decoders: Wazuh v1.0.1 to v1.1.1
        old_decoders="ossec_decoders wazuh_decoders"
        for old_decoder in $old_decoders
        do
            if [ -d "$WAZUH_HOME/etc/$old_decoder" ]; then
                mv "$WAZUH_HOME/etc/$old_decoder" $BACKUP_RULESET
            fi
        done

        # Backup decoders: Wazuh v1.0 and OSSEC
        if [ -f "$WAZUH_HOME/etc/decoder.xml" ]; then
            mv "$WAZUH_HOME/etc/decoder.xml" $BACKUP_RULESET
        fi

        # Backup rules: All versions
        mv "$WAZUH_HOME/rules" $BACKUP_RULESET

        # New ossec.conf by default
        ./gen_ossec.sh conf "manager" $DIST_NAME $DIST_VER > $OSSEC_CONF_FILE
        ./add_localfiles.sh $WAZUH_HOME >> $OSSEC_CONF_FILE
    else
        # New ossec.conf by default
        ./gen_ossec.sh conf "agent" $DIST_NAME $DIST_VER > $OSSEC_CONF_FILE
        # Replace IP
        ./src/init/replace_manager_ip.sh $OSSEC_CONF_FILE_ORIG $OSSEC_CONF_FILE
        ./add_localfiles.sh $WAZUH_HOME >> $OSSEC_CONF_FILE
    fi
}
