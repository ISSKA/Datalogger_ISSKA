Ce programme Arduino est tr�s important car il permet de mesurer la consommation de n'importe quel autre Arduino. Le montage est expliqu� sur le sch�ma Fritzing.
On peut suivre la consommation via le moniteur de s�rie (Serial Monitor), ou alors en utilisant CoolTerm. L'avantage de CoolTerm est qu'on peut sauvgarder les
donn�es transmises via la connexion USB (onglet "Connection/capture to text file") et ensuite faire un plot en utilisant le script Python "plot.py". Il faut juste changer
le nom de l'input file � la ligne 8 (ici "exemple.asc").