class Node:
    def __init__(self, name):
        self.name = name
        self.edges = []

    def addEdge(self, to, bi = False):
        self.edges.append(to)
        if bi:
            to.addEdge(self)

movies = {}
stars = {}

with open('input-mpaa.txt', 'r', encoding='cp1252') as file:
    lines = file.read().split('\n')
    for line in lines:
            split = line.split('/')
            if len(split) > 1:
                movie = Node(split[0])
                movies[split[0]] = movie
                for i in range(1, len(split)):
                    if split[i] not in stars:
                        star = Node(split[i])
                        stars[split[i]] = star
                        movie.addEdge(star, True)


print(len(movies), len(stars))