#!/bin/bash
bear --output "../misc/compile_commands.json" -- make SILENT=@ DEBUG=1 DEVELOPER_BUILD=1
