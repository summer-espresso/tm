#! /bin/sh
### BEGIN INIT INFO
# Provides:          tm
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Task Manager
# Description:       This file should be used to construct scripts to be
#                    placed in /etc/init.d.
### END INIT INFO

# Author: Foo Bar <foobar@baz.org>
#
# Please remove the "Author" lines above and replace them
# with your own name if you copy and modify this script.

# Do NOT "set -e"

# PATH should only include /usr/* if it runs after the mountnfs.sh script
PATH=/sbin:/usr/sbin:/bin:/usr/bin
DESC="Task Manager"
NAME=tm
DAEMON=/usr/bin/$NAME
DAEMON_ARGS=""
PIDDIR=/tmp
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/$NAME
WORKINGDIR=/usr/share/tm

# Exit if the package is not installed
[ -x "$DAEMON" ] || exit 0

# Read configuration variable file if it is present
[ -r /etc/default/$NAME ] && . /etc/default/$NAME

# Load the VERBOSE setting and other rcS variables
. /lib/init/vars.sh

# Define LSB log_* functions.
# Depend on lsb-base (>= 3.2-14) to ensure that this file is present
# and status_of_proc is working.
. /lib/lsb/init-functions

do_start()
{
	# Return
	#   0 if daemon has been started
	#   1 if daemon was already running
	#   2 if daemon could not be started

	if [ -f "$PIDDIR/$NAME.pid" ] ; then
		PIDFOUND=$(cat $PIDDIR/$NAME.pid)
		PIDMEMORY=$(ps h -p $PIDFOUND | grep $NAME | awk '{print $1}')
		if [ "$PIDFOUND" = "$PIDMEMORY" ] ; then
			return 1
		fi
	fi

	EXEC_USER=""

	if [ "$DAEMON_USER" != "" ] ; then
		EXEC_USER="-c $DAEMON_USER"
	fi

	if [ "$DAEMON_GROUP" != "" ] ; then
		EXEC_GROUP="-g $DAEMON_GROUP"
	fi

	if [ "$APP_PORT" != "" ] ; then
		DAEMON_ARGS="$DAEMON_ARGS -p $APP_PORT"
	fi

	if [ "$APP_STORAGE_PATH" != "" ] ; then
		DAEMON_ARGS="$DAEMON_ARGS -s $APP_STORAGE_PATH"
	fi

	if [ "$APP_TITLE" != "" ] ; then
		DAEMON_ARGS="$DAEMON_ARGS -t $APP_TITLE"
	fi

	if [ "$APP_LOG_LEVEL" != "" ] ; then
		DAEMON_ARGS="$DAEMON_ARGS -l $APP_LOG_LEVEL"
	fi

	if [ "$APP_KILL_SIGNAL" != "" ] ; then
		DAEMON_ARGS="$DAEMON_ARGS -k $APP_KILL_SIGNAL"
	fi

	start-stop-daemon --start $EXEC_USER $EXEC_GROUP --quiet --background --make-pidfile --pidfile $PIDDIR/$NAME.pid --chdir $WORKINGDIR --exec $DAEMON -- $DAEMON_ARGS
	retval=$?
	return ${retval}
}

do_stop()
{
	# Return
	#   0 if daemon has been stopped
	#   1 if daemon was already stopped
	#   2 if daemon could not be stopped
	#   other if a failure occurred

	if [ -f "$PIDDIR/$NAME.pid" ] ; then
		start-stop-daemon --stop --quiet --pidfile $PIDDIR/$NAME.pid
		retval=$?
		return ${retval}
	fi

	return 1
}

do_status()
{
	if [ -f "$PIDDIR/$NAME.pid" ] ; then
		PIDFOUND=$(cat $PIDDIR/$NAME.pid)
		PIDMEMORY=$(ps h -p $PIDFOUND | grep $DAEMON | awk '{print $1}')
		if [ "$PIDFOUND" = "$PIDMEMORY" ] ; then
			return 1
		fi
	fi

	return 0
}

case "$1" in
  start)
	[ "$VERBOSE" != no ] && log_daemon_msg "Starting $DESC" "$NAME"
	do_start
	case "$?" in
		0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
		2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
	esac
	;;
  stop)
	[ "$VERBOSE" != no ] && log_daemon_msg "Stopping $DESC" "$NAME"
	do_stop
	case "$?" in
		0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
		2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
	esac
	;;
  status)
	do_status && exit 0 || exit $?
	;;
  restart)
	log_daemon_msg "Restarting $DESC" "$NAME"
	do_stop
	case "$?" in
	  0|1)
		do_start
		case "$?" in
			0) log_end_msg 0 ;;
			1) log_end_msg 1 ;; # Old process is still running
			*) log_end_msg 1 ;; # Failed to start
		esac
		;;
	  *)
		# Failed to stop
		log_end_msg 1
		;;
	esac
	;;
  *)
	echo "Usage: $SCRIPTNAME {start|stop|status|restart}" >&2
	exit 3
	;;
esac

:
