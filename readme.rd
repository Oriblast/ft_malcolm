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

sudo ./ft_malcolm 10.0.2.4 08:00:27:9d:1c:5a 10.0.2.15  08:00:27:26:ed:89 