import piles
solver = piles.PileSolver()
tree = solver.build_diophantine_tree(csv_path="diophantine.txt", tree_path='tree.bin')
