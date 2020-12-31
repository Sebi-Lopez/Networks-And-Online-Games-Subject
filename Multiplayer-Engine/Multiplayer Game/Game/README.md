# Networks and Online Games Subjet

# Multiplayer Game in C++

## Group members

- José Antonio Prieto 
- Sebastià López

## GAME NAME

GAMENAME is a 2D shooter where you compete agains other players online in a firing range.
You are at the far west, and there has been declared a Shooting competition in the tabern of a small city. 
You must kill the bad guys that  appear in the windows of the tabern before your oponents! The more YOU kill, the more points for you in the competition!
But be carefull! There are innocent people too! You must  avoid killing them in order to not lose your points!

## Instructions

Mouse: Aim
Mouse Left Button: Shoot 

Objective: 
Kill the bad guys.
Not kill innocent people.

When all players are ready to start, click on i'm ready! checkbox
on Player info panel, the game starts immediately for all connected players!

## List of Implemented features 

- World State Replication 
- Authors: 
José Antonio Prieto
Sebastià Lopez
- State
Completed.

- Reliability On Top of UDP 
- Authors:
Sebastià López
- State
Completed. 
- Known bugs
When losing a package that tells you you hit an enemy, once it arrives again, the blood splash (the hit  marker) in the client will appear in the current position of the mouse, not the  position of the hit.

- Client Side Prediction
- Authors:
José Antonio Prieto
- State 
Reworked. 
In our specific game, we didn't think the client needed the server side reconciliation. As there are no possible objects or events that happen to the "player" (crosshair) in the server that the client can't acknowladge. 
So, the input of each client is handled locally, and sent it periodically to the server that Updates the position to all the other clients. 

- Entity Interpolation
- Authors:
Sebastian Lopez Tenorio:
- State:
Completed.
In our  game the interpolation is not as visible as in  the spaceship game. The  system works, but due to the high acceleration and the high velocity of the mouse movements, the interpolation is harder to notice.
If you switch to the branch called "Entity Interpolation" you will see the accomplished feature and its effect more clearly in the spaceship game. 

- Lag Compensation
- Authors:
Sebastian López Tenorio
José Antonio Prieto
- State: 
Tried but  not accomplished.
Started implementing the structure where we would store the past states of the windows (where the npcs spawn). 
Started implementing a system that would synchronize both client clocks and server clocks so we would know in what specific time of the client was sent an input (in reference to the Server's clock). Here we started having some trouble: 
We started by sending the time the server was connected to the server so we would then substract that time in the server and know exactly what delay was the client's clock relative to the server's clock. But we encountered the problem that there could always be a package delay that would not grant us that that calculated delay was actually real. 
We thought that maybe sending perodical packages in the update to calculate an average of that delay would be a good solution.
We digged into it in the Internet, and we discovered that its a bigger issue than we expected. But we got a good hint that the previous method would probably work.
With this problem, and the complexity that would come afterwards in looking in the past states to then modify all the present states (and all those problems that may arise), and with not much time left, we decided to not keep going with this and focus on the game instead. 


