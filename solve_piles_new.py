import build.piles as piles
import numpy as np
import time
import argparse
import tqdm

def sum_cube(row, pile_num):
    return sum((i+1)**3 for i, val in enumerate(row) if val==pile_num)

def dedupe_mask(assigned_piles):
    # If we end up with multiples of the the same disallowed "mask" (e.g. the same numbers are taken, but in different
    # piles) we only need to keep one of them. This just cuts down on the number of downstream piles we have to consider.
    mask = assigned_piles!=-1
    # Use numpy's lexsort to find first occurrence of each unique mask
    _, idx = np.unique(mask, axis=0, return_index=True)
    # print('before')
    # print(assigned_piles.shape)
    assigned_piles = assigned_piles[idx]
    # print('after')
    # print(assigned_piles.shape)
    return assigned_piles

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Solve the piles problem')
    parser.add_argument('--n_piles', type=int, default=9,
                      help='Number of piles (default: 9)')
    parser.add_argument('--n_cubes', type=int, default=53,
                      help='Number of cubes (default: 53)')
    parser.add_argument('--chunk_size', type=int, default=10,
                      help='How many entries in the first pile to dfs')
    parser.add_argument('--no_progress', action='store_true',
                      help='Disable progress bars')
    parser.add_argument('--no_memoize', action='store_true')
    parser.add_argument('--no_diophantine', action='store_true')
    parser.add_argument('--no_mask_dedupe', action='store_true')
    parser.add_argument('--early_exit', action='store_true', help='Exit as soon as we find a solution')
    args = parser.parse_args()

    solver = piles.PileSolver(num_piles=args.n_piles, 
                            num_cubes=args.n_cubes, 
                            do_memoize=not args.no_memoize,
                            do_diophantine=not args.no_diophantine,
                            tree_path=f'/Users/rvilim/repos/spheres/filters/tree_{((args.n_cubes + 4) // 5) * 5}_10.bin', 
                            memoization_path=f'/Users/rvilim/repos/spheres/memo.bin',
                            memoization_limit=26)
    
    start_time = time.time()

    assigned_piles = np.array([solver.init_distribution()])
    first_pile = solver.solve_from_assignment(assigned_piles, 0, num_threads=1)
    chunk_size = args.chunk_size
    num_chunks = (len(first_pile) + chunk_size - 1) // chunk_size
    chunks = np.array_split(first_pile, num_chunks)

    all_solutions = []
    for chunk_idx, chunk in tqdm.tqdm(enumerate(chunks), total=len(chunks), 
                                     desc="Processing chunks", disable=args.no_progress):
        assigned_piles = chunk

        for pile_num in tqdm.tqdm(range(1, args.n_piles), desc=f"Solving pile", 
                                 leave=False, disable=args.no_progress):
            if pile_num==1:
                assigned_piles = solver.solve_from_assignment(assigned_piles, pile_num, num_threads=12, do_mask_dedupe=not args.no_mask_dedupe)
            else:
                assigned_piles = solver.solve_from_assignment(assigned_piles, pile_num, num_threads=12, do_mask_dedupe=False)
        if len(assigned_piles) > 0:
            all_solutions.append(assigned_piles)
            if args.early_exit:
                break
    total_time = time.time() - start_time

    if all_solutions:
        final_solutions = np.concatenate(all_solutions)
        print(" ")
        print(f"Total solutions found: {len(final_solutions)}")
        print(f"Total time: {total_time:.2f}s")
        print(" ")

        # for i in range(final_solutions.shape[0]):
        #     print_piles(final_solutions[i,:], args.n_piles, args.n_cubes)
        #     print(" ")
    else:
        print("\nNo complete solutions found")

def print_piles(solution, n_piles, n_cubes):
    for pile in range(n_piles):
        s = sum((pos+1)**3 for pos, s in enumerate(solution) if s==pile)
        print(''.join(str(int(pile==s)) for s in solution),'-',s)
        
def test():
    parser = argparse.ArgumentParser(description='Solve the piles problem')
    parser.add_argument('--memoize', action='store_true')
    args = parser.parse_args()

    solver = piles.PileSolver(num_piles=8, 
                            num_cubes=47, 
                            do_memoize=args.memoize,
                            do_diophantine=False,
                            tree_path=f'/Users/rvilim/repos/spheres/filters/tree_{((47 + 4) // 5) * 5}_10.bin', 
                            memoization_path=f'/Users/rvilim/repos/spheres/memo.bin',
                            memoization_limit=26)
    
    assigned_piles = np.array([solver.init_distribution()])
    first_pile = solver.solve_from_assignment(assigned_piles, 0, num_threads=1)
    second_pile = solver.solve_from_assignment(first_pile, 1, num_threads=1)

    # str_array = np.array(second_pile, dtype=str)
    # str_array[str_array == '-1'] = '.'
    # for row in str_array:
    #     print(''.join(row))
        
    # print(first_pile.shape)
    # print(second_pile)
    # print(first_pile[(first_pile[:,2]==0) & (first_pile[:,3]==0)].shape)
    # print(second_pile.shape)
if __name__ == "__main__":
    main()