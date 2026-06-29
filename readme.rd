parse des arguments

↓

conversion des adress ip et mac en binaire

↓

Création du socket AF_PACKET

↓

Construction de l'en-tête Ethernet

↓

Construction du paquet ARP

↓

recvfrom()
(attendre une request)

↓

sendto()
(avec sockaddr_ll)