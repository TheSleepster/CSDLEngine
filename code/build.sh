#!/bin/bash
bear --output "../misc/compile_commands.json" -- make -k SILENT=@ DEBUG=1 ASSERT_ENABLED=1 DEVELOPER_BUILD=0
