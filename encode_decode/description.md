Le projet prétexte encoder-decoder vous occupera en attendant que le gros
projet MAS soit prêt. MAS est très excitant; mais il est complexe aussi. Il
nécessitera donc que vos tech leads vous servent quelques trucs sur un plateau
d'argent; et ça prendra un peu de temps.

# Introduction au concept de codec
Il y a des media de communication digitale qui n'acceptent que du texte; et
du texte ASCII en plus. Donc:

- Les accents et cédilles sont interdits
- Les images sont interdites
- Le son est interdit
- Les videos sont interdits
- Les fichiers exécutables sont interdits, etc.

Savez-vous comment vous pouvez tromper un tel système? En trouvant une fonction
bijective qui, à chaque texte avec accents et/ou cédiles, à chaque video, à
chaque son, à chaque exécutable, etc. associe une transformation en texte qui
est unique. En fait, chaque transformation est si unique qu'il serait possible
de renverser le processus et d'obtenir exactement le fichier son, video,
exécutable, etc. initial!

Le processus qui transforme votre fichier bizarre et interdit en texte ASCII
s'appelle **encodage** (ou *coding*, ou *encoding*). 
Le fichier ainsi obtenu s'appelle 
*fichier encodé*. Le processus inverse, qui transforme un fichier encodé en
son équivalent initial s'appelle **décodage** (ou decoding).

Il y a plein plein plein de encoding-decoding qui se font dans la vie et autour
de vous. Votre ordinateur ne manipule que des bits. Donc tout ce qui y entre
est d'abord encodé; puis tout ce qui en sort sous forme de son, video et texte
est décodé pour que vous puissiez le comprendre. Plein d'autres exemples:

- Le signal du téléphone
- Canal+ (C'est pour ça que vous avez besoin d'un décodeur)
- Le wifi: Le signal radio est décodé en bits en entrant dans votre ordi ou
	téléphone; puis encodé en signal radio en en sortant.
- etc.

parlez-moi de quelque chose; et je vous parlerai d'un probable encodage
et décodage qui s'y applique.


Un **codec** (**co**der-**dec**oder) est l'implantation concrète de la
fonction (possiblement bijective) 
qui permet de faire un encodage-décodage. Il y a des codecs
différents pour différentes choses. Il y a un codec pour les sons MP3, il y en 
a un autre pour le format son "ogg vorbis", etc. Il y a aussi d'autres pour les
video MP4; et d'autres pour les WMV, etc. Il peut avoir plus d'un codec pour le
même type de transformation (ex: bits à vidéo); mais pour décoder le fichier
encodé par le codec X, il faut nécessairement le codec X. En d'autres termes,
le décodeur du codec Y ne peut pas décoder un fichier encodé par l'encodeur du
codec X. Finalement, deux codecs C1 et C2
peuvent faire le même travail (même format source et même format destination)
et être différents sur plusieurs paramètres. Par exemple C1 peut être beaucoup
plus rapide en encodage que C2; mais consommer beaucoup d'électricité en
contrepartie. Les codecs servent à:

- Régler des problèmes de format. Ils agissent comme traducteurs entre 2
	entités qui ne parlent pas le même langage. Exemple, Image à ASCII ou signal
	radio à bits.
- Empêcher des gens de lire une donnée à moins que vous leur en donniez la
	permission. Exemple Canal+. 

#Votre travail
Votre mission, si toutefois vous l'acceptez, consistera à écrire des codecs.
Les codecs seront dans un programme auquel on pourra spécifier le codec désiré.
Dans ce qui suit on désignera un codec par le triplet (id, C, D) tel que:

- *id* est l'identificateur du codec. Disons que c'est un entier unique dans le
	contexte de votre programme.
- *C* est l'encodeur. D'un point de vue conceptuel, il peut être considéré 
	comme
	une fonction mathématique (souffrez que je fasse quelques simplifications
	parce-que si on veut être rigoureux, une vraie fonction mathématique ne
	détruit jamais la variable qu'elle prend en entrée ... contrairement à ce que
	fait un encodeur ou un décodeur)
- *D* est le décodeur qui va avec l'encodeur *C*. *D* peut ausi être vu comme 
		une fonction mathématique.

Pour chaque fichier f pour lequel C(f) = F, on doit avoir D(F) = f. En gros,
(DoC(f) = f.

Donc si votre programme s'appelle enc_dec. Vous aurez les scénarios
d'exécution suivants:

- ./enc_dec -h
	- Afficher un message d'aide pour expliquer ce que le programme fait. Ce
		message doit aussi expliquer chaque option.
- ./enc_dec -l
	- Lister les *id*s des codecs présents. Pour chaque *id*, expliquer ce que
    l'**encodeur** fait. Exemple:
		<pre style="color:darkblue; font-family:courier"> 
    <b>./enc_dec -l</b>
       0: Inverse chaque byte (octet)
       1: Inverse chaque lot de 16 bytes. Si le dernier lot est moins 
          de 16 bytes, il ne l'inverse pas.
       2: Applique un XOR (exclusive OR) sur chaque byte en utilisant le mask
          11111111 (en binaire) ou 0xFF (en hexadecimal)
       3: Applique un XOR sur chaque lot de 4 bytes avec le pattern 
          0xDEADBEEF. Si le dernier lot est moins de 4 bytes, il l'inverse.
       4: Ajoute le pattern 0xBABEFACE à chaque lot de 512 bytes. Si le dernier
          lot est moins de 512, il lui fait un XOR byte par byte avec le 
          pattern 0xFF.
	</pre>
- <span style="font-family:courier">
./enc_dec -e id -i INPUT_FILE -o OUTPUT_FILE</span>
	- Encoder le fichier *INPUT_FILE* avec l'encodeur du codec *id*. Le fichier
		résultant s'appelle *OUTPUT_FILE*.
- <span style="font-family:courier"> 
./enc_dec -d id -i INPUT_FILE -o OUTPUT_FILE</span>
	- Décoder le fichier *INPUT_FILE* avec le décodeur du codec *id*. Le fichier
		résultant s'appelle *OUTPUT_FILE*.
	
Quelques infos supplémentaires:

1. Si vous êtes dans une équipe de *n* membres, votre programme devrait avoir 
	au moins *2n* codecs. Plus de codecs est encore mieux :-)
- Vos codecs devraient pouvoir encoder et décoder **n'importe quel type de
	fichier** (texte, son, video, fichier binaire, ABSOLUMENT TOUT). 
	- En gros, si je prends une vidéo que j'encode et que je rédécode avec le
		même codec id, la vidéo doit se remettre à fonctionner.
	- Un fichier exécutable f auquel on applique D(C(f)) doit encore s'exécuter
		de la même manière etc.
- Au fond, le point ci-dessus se fait tout seul si votre encodage se fait au
	niveau binaire.
- Utilisez votre imagination pour inventer des algorithmes d'encodage. Ce qui
	est listé avec "<span style="font-family:courier">./enc_dec -l</span>" peut
	servir d'inspiration de départ. Chacun des algorithms d'encodage listés dans
	cet exemple est réversible (bijectif); donc vous devez aussi penser à des
	algos réversibles.
- Ce projet est à faire dans le langage de choix de votre équipe!

## Comment tester?
Suposons que f' = C(D(f)). On veut vérifier que f'==f. 
Il y a 2 manières de le faire.

### La manière des gens "ordinaires"
Comparer visuallerment f et f'. Si f est un son, vidéo ou exécutable, 
jouer ou exécuter f et f' pour voir si c'est le même son, vidéo ou exécutable.

### La manière des (futurs) ingénieurs.
Premièrement, sachez que beaucoup de formats de fichiers ont des entêtes ou des
"queues" de *n* bytes. C'est souvent un entête. Au début, le lecteur (ex:
Windows media player pour une vidéo) lit l'entête pour reconnaître le fichier.
Si l'entête est corrompu, alors la lecture échoue dès le début. Mais si c'est
un fichier immense et que la corruption ne commence qu'après 90% du fichier,
vous ne saurez pas que le fichier est corrompu avant longtemps dans votre
comparaison. Donc ...

... l'approche des "gens ordinaires" est de piètre qualité pour plusieurs
raisons:

- Vous ne pourrez pas déclarer que votre truc marche en visualisant seulement
	10% d'une video de 2h. Vous auriez des faux-positifs.
- Et vous ne voulez surtout pas visualiser une vidéo de 2h. Sérieusement,
	pourquoi perdriez-vous tout ce temps?

N'utilisez pas la force et le temps d'un humain pour faire le travail, 
utilisez la machine *et donnez-lui des ordres*. Utilisez:

- Le programme *diff* par exemple
- Mais le truc que je recommanderais serait de calculer le checksum *s* de *f*; 
	et
	ensuite le checksum de *s'* de *f'*; et je comparerais *s* et *s'*.
	- Je suis sûr, qu'étant pour la plupart en comms et réseaux, vous savez tous 
		ce qu'est un checksum. Si ce n'est pas le cas, "demandez" à Google.
	- Il y a des outils pour calculer les checksum. Il y a `openssl` et `shasum`.
		Pour calculer par exemple le SHA sur 256 bits (un type de checksum) 
		du fichier *FILE_NAME*, vous pouvez faire:
		- `shasum -a 256 FILE_NAME`

### Attention!
Pendant que vous faites vos tests, utilisez des fichiers non sensibles ou
des copies de fichiers (si les fichiers sont importants). Ainsi, par exemple:

- Si vous encodez l'exécutable de Google-chrome
- Et il a un bug dans votre code
- Et votre décodeur ne peut pas refaire le Google-chrome original
- Vous ne vous retrouverez pas en larmes.

<pre> ----------------------------------</pre>

Pour votre information, un codec n'est pas obligé d'être bijectif. Le codec de
Canal+ n'est probablement pas bijectif parce-que vous n'avez probablement 
jamais besoin de faire par exemple C(D(C(f))). Le décodage est
	terminal! Un fichier décodé est consommé par vos yeux et vos oreilles et
	c'est la fin de l'histoire. Il n'y a probablement pas de scénario où le
	fichier décodé sera réencodé. Par conséquent, l'encodeur peut être *à perte*
	pour des raisons de performance. Par exemple, si on sait que l'oreille
	humaine ne peut pas percevoir certaines fréquences de son, *C* peut simplement
	les ignorer pendant l'encodage et elles sont perdues à jamais. Cette perte ne
	sera pas perçue dans le fichier décodé "lu" par l'humain; mais ce fichier
	décodé n'est plus le même que le fichier initial.

