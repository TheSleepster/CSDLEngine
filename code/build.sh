#!/bin/bash
bear --output "../misc/compile_commands.json" -- make -k SILENT=@ DEBUG=1 DEVELOPER_BUILD=0
