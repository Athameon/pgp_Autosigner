#!/usr/bin/expect
spawn gpg --edit-key 0x6B7A6BBE sign save
expect "Really sign all user IDs? (y/N)"
send -- "y\n"
expect "Really sign? (y/N)"
send -- "y\n"
expect "Really sign? (y/N)"
send -- "\n"

interact