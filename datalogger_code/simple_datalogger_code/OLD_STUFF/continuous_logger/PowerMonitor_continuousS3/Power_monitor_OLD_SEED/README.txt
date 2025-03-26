Ce programme Arduino est très important car il permet de mesurer la consommation de n'importe quel autre Arduino. Le montage est expliqué sur le schéma Fritzing.
On peut suivre la consommation via le moniteur de série (Serial Monitor), ou alors en utilisant CoolTerm. L'avantage de CoolTerm est qu'on peut sauvgarder les
données transmises via la connexion USB (onglet "Connection/capture to text file") et ensuite faire un plot en utilisant le script Python "plot.py". Il faut juste changer
le nom de l'input file à la ligne 8 (ici "exemple.asc").