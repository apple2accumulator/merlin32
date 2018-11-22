#!/bin/sh

BIN_DIR=`dirname "$0"`
if [ "$BIN_DIR" = "." ]
then
	BIN_DIR=`pwd`
fi

# Get PREFIX, INSTALL_BIN, MACRO_INSTALL_DIR
. "$BIN_DIR"/config.sh

TOP_SOURCE_DIST_DIR=`dirname "$0"`

install -d "$INSTALL_BIN"
install "$TOP_SOURCE_DIST_DIR"/Source/merlin "$INSTALL_BIN"
install -d "$MACRO_INSTALL_DIR"
install "$TOP_SOURCE_DIST_DIR"/Library/* "$MACRO_INSTALL_DIR"

