<?php include "header.php"; ?>
<p>
This site is about WD6AWP and assoicated <a href="http://allstarlink.org" target="_blank">Allstar</a> nodes. 
</p>

<ul>
<li>
Node 2530 is a hub type of node - a conference bridge. There is no radio wired to it. 
</li>

<li>
Node 2532 is a two meter repeater system with a simulcast/voter on 145.140- P/L 110.9Hz. 
</li>
<li>
Voter 2532 shows the voter of the two meter repeater.
</li>

<li>
And some other nodes... 
</ul>

<p>
On the menu bar click on the node numbers to see, and manage if you have a login ID, each local node. 
These pages dynamically display any remote nodes that are connected to it. 
When a signal is received the remote node will move to the top of the list and will have a green background. 
The most recently received nodes will always be at the top of the list. 
<ul>
<li>
The <b>Direction</b> column shows IN when another node connected to us and OUT if the connection was made from us. 
</li>
<li>
The <b>Mode</b> column will show Transceive when this node will transmit and receive to/from the connected node. It will show Rx only if this node only receives from the connected node.
</li>
</ul>
</p>

<p>
Any Voter page(s) show will receiver details. The bars will move in near-real-time as the signal strength varies. 
The voted receiver will turn green indicating that it is being repeated.
The numbers are the relative signal strength indicator, RSSI. The value ranges from 0 to 255, a range of approximately 30db.
A value of zero means that no signal is being received. 
The color of the bars indicate the type of RTCM client as shown on the key below the voter display.
This system has four voting receivers and two faux-simulcast transmitters, as of this writing.
The main transmitter is on Santiago Peak. The second is a low power transmitter providing coverage to the Huntington Beach EOC. The receiver locations are shown on the voter page with their name/location under the client column. Master in the client name means that it is the timing source for the system. All radio sites are connected over IP networks using the Allstar <a href="http://micro-node.com/thin-m1.html" target="_blank">RTCM - Radio Thin Client Module</a>.
</p>
<?php include "footer.php"; ?>