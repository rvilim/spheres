import build.piles as piles
import numpy as np
import time
import argparse
import tqdm

def sum_cube(row, pile_num):
    return sum((i+1)**3 for i, val in enumerate(row) if val==pile_num)

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Solve the piles problem')
    parser.add_argument('--n_piles', type=int, default=9,
                      help='Number of piles (default: 9)')
    parser.add_argument('--n_cubes', type=int, default=53,
                      help='Number of cubes (default: 53)')
    parser.add_argument('--chunk_size', type=int, default=1,
                      help='How many entries in the first pile to dfs')
    args = parser.parse_args()

    solver = piles.PileSolver(num_piles=args.n_piles, 
                            num_cubes=args.n_cubes, 
                            do_memoize=True,
                            do_diophantine=True,
                            tree_path=f'/Users/rvilim/repos/spheres/filters/tree_{((args.n_cubes + 4) // 5) * 5}_10.bin', 
                            memoization_path=f'/Users/rvilim/repos/spheres/memo.bin',
                            memoization_limit=26)

    assigned_piles = np.array([solver.init_distribution()])
    first_pile = solver.solve_from_assignment(assigned_piles, 0, num_threads=12)

    chunk_size = args.chunk_size
    num_chunks = (len(first_pile) + chunk_size - 1) // chunk_size
    chunks = np.array_split(first_pile, num_chunks)

    all_solutions = []
    for chunk_idx, chunk in tqdm.tqdm(enumerate(chunks), total=len(chunks), desc="Processing chunks"):
        assigned_piles = chunk
        
        for pile_num in tqdm.tqdm(range(1, args.n_piles), desc=f"Solving pile", leave=False):
            assigned_piles = solver.solve_from_assignment(assigned_piles, pile_num, num_threads=12)
        
        if len(assigned_piles) > 0:
            all_solutions.append(assigned_piles)
            break

    if all_solutions:
        final_solutions = np.concatenate(all_solutions)
        print(f"\nTotal solutions found: {len(final_solutions)}")
        print_piles(final_solutions[0,:], args.n_piles, args.n_cubes)
    else:
        print("\nNo complete solutions found")


def print_piles(solution, n_piles, n_cubes):
    for pile in range(n_piles):
        s = sum((pos+1)**3 for pos, s in enumerate(solution) if s==pile)
        print(''.join(str(int(pile==s)) for s in solution),'-',s)
        

if __name__ == "__main__":
    main()