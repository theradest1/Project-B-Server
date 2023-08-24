# Project-B-Server
A c++ game server that uses both UDP and TCP. 

I made this specifically to go along with my universal multiplayer client made in unity. 
It uses UDP for movement, and game states that are updated constantly to reduce bandwidth and so that there arent duplicate messages sent if one doesnt make it to the target.
Along with that, it also uses TCP for the event system and important messages.

This server is mainly event based, while also being more of a phone line, where it processes only specified messages and just transferes the rest. I intend to have it efficient enough that it could run on basically anything (like a raspberry pi) with a relativly high player count.

There is basically no security, and I don't ever intend on adding any security to this further than a basic encryption.
