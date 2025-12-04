savedcmd_tsune.mod := printf '%s\n'   tsune.o | awk '!x[$$0]++ { print("./"$$0) }' > tsune.mod
