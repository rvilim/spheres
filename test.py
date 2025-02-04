import piles
import argparse
import json
from collections import defaultdict
import pickle
import tqdm
import time

# def read_diophantine(pickle_filename = 'diophantine.pkl', text_filename='diophantine.txt'):
#     try:
#         with open(pickle_filename, 'rb') as f:
#             p = pickle.load(f)
#             a=sorted([(k,len(v)) for k,v in p.items()], key = lambda g: g[1], reverse=True)
#             for row in a:
#                 print(row)
#             return p
        
#     except FileNotFoundError:
#         pass

#     print("diophantine pickle not found, reading", text_filename)
#     diophantine = defaultdict(list)
#     with open(text_filename) as f:
#         for line in f:
#             line_split=line.strip().split(',')
#             target = int(line_split[-1])
#             body_bits=0
#             for bit in line_split[:-1]:
#                 body_bits |= 1<<(int(bit)-1)
#             diophantine[target].append(body_bits)

#     with open(pickle_filename, 'wb') as f:
#         pickle.dump(diophantine, f)
#     return diophantine

def print_piles(combination, n_piles, n_cubes):
    ok = True
    out=''
    for c in combination:
        conflict = any(sum(k)!=1 for k in zip(*combination))

        bin_repr = ''.join(map(str, c))
        actual_sum = sum((i+1)**3 for i, chosen in enumerate(c) if chosen==1)
        ideal_sum = int((n_cubes*(n_cubes+1)/2)**2)//n_piles

        ok &= (not conflict)
        ok &= (ideal_sum == actual_sum)

        print(f'{bin_repr} - {actual_sum}/{ideal_sum}/{("no conflict" if not conflict else "conflict")}')

    for pos in range(n_cubes):
        for pile_num, pile in enumerate(combination):
            if pile[pos]==1:
                out+=str(pile_num+1)
    print("----------")
    print(out)
    if not ok:
        print(" ")
        print("❌❌❌")
    print(" ")
    print(" ")
    return ok

# def filter_diophantine(next_piles, disallowed, diophantine):
#     filtered_piles = []
#     for pile in next_piles:
#         pile_bits = pile_to_bin(pile)
#         is_valid = True

#         # Check each bit position
#         for pos in range(len(pile)): 
#             bit = 1 << pos
#             # If this bit is not set in pile_bits and not disallowed
#             if not (pile_bits & bit) and not (disallowed & bit):
#                 filters = diophantine[pos+1]
#                 for filter in filters:
#                     # If all bits in the filter are set in pile_bits
#                     if filter & pile_bits == filter:
#                         # This means we found a combination of lower bits
#                         # that sum to the same value as the current position
#                         is_valid = False
#                         print(pos, bin(filter))

#                         break
#             if not is_valid:
#                 break
                
#         if is_valid:
#             filtered_piles.append(pile)

#     return filtered_piles

def pile_to_bin(pile):
    binpile = 0
    for i, p in enumerate(pile):
        if p:
            binpile |= (1 << i)
    return binpile

def get_disallowed(piles):
    disallowed = 0

    for pile in piles:
        disallowed |=pile_to_bin(pile)

    return disallowed

def solve(n_piles, n_cubes, diophantine=None):
    sums = [1, 9, 36, 100, 225, 441, 784, 1296, 2025, 3025, 4356, 6084, 8281, 11025, 14400, 18496, 23409, 29241, 36100, 44100, 53361, 64009, 76176, 90000, 105625, 123201, 142884, 164836, 189225, 216225, 246016, 278784, 314721, 354025, 396900, 443556, 494209, 549081, 608400, 672400, 741321, 815409, 894916, 980100, 1071225, 1168561, 1272384, 1382976, 1500625, 1625625, 1758276, 1898884, 2047761, 2205225, 2371600, 2547216, 2732409, 2927521, 3132900, 3348900, 3575881, 3814209, 4064256, 4326400, 4601025, 4888521, 5189284, 5503716, 5832225, 6175225, 6533136, 6906384, 7295401, 7700625, 8122500, 8561476, 9018009, 9492561, 9985600, 10497600, 11029041, 11580409, 12152196, 12744900, 13359025, 13995081, 14653584, 15335056, 16040025, 16769025, 17522596, 18301284, 19105641, 19936225, 20793600, 21678336, 22591009, 23532201, 24502500, 25502500]
    cubes = [1, 8, 27, 64, 125, 216, 343, 512, 729, 1000, 1331, 1728, 2197, 2744, 3375, 4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167, 13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791, 32768, 35937, 39304, 42875, 46656, 50653, 54872, 59319, 64000, 68921, 74088, 79507, 85184, 91125, 97336, 103823, 110592, 117649, 125000, 132651, 140608, 148877, 157464, 166375, 175616, 185193, 195112, 205379, 216000, 226981, 238328, 250047, 262144, 274625, 287496, 300763, 314432, 328509, 343000, 357911, 373248, 389017, 405224, 421875, 438976, 456533, 474552, 493039, 512000, 531441, 551368, 571787, 592704, 614125, 636056, 658503, 681472, 704969, 729000, 753571, 778688, 804357, 830584, 857375, 884736, 912673, 941192, 970299, 1000000]

    solver = piles.PileSolver()
    assigned_piles = solver.init_distribution(n_piles, n_cubes)
    start_pos = solver.init_pos(assigned_piles)
    assigned_remaining = solver.init_remaining(assigned_piles, n_piles)

    disallowed = 0
    # Initialize list to store all pile combinations
    pile_combinations = [[]]
    # Iterate through all piles
    nums = {}
    for pile_idx in range(n_piles):
        s = 0
        new_combinations = []
        # For each existing combination of piles
        for prev_piles in pile_combinations:
            # Calculate target and remaining for next pile
            if pile_idx == 0:
                target = assigned_remaining[0]
                remaining = sums[n_cubes-1]
                assigned_pile = assigned_piles[0]
                curr_disallowed = get_disallowed(assigned_piles[1:])
            else:
                target = assigned_remaining[pile_idx]
                preassigned_disallowed = 0
                if pile_idx != n_piles-1:
                    preassigned_disallowed = get_disallowed(assigned_piles[(pile_idx+1):])
                curr_disallowed = get_disallowed(prev_piles) | preassigned_disallowed
                remaining = solver.calc_remaining(curr_disallowed, n_cubes)
                assigned_pile = assigned_piles[pile_idx]
            
            # Generate next pile
            next_piles = solver.make_pile(target, remaining, start_pos, assigned_pile, curr_disallowed)

            # if pile_idx < 1:
            #     before = len(next_piles)
            #     start_time = time.time()
            #     next_piles = filter_diophantine(next_piles, curr_disallowed, diophantine)
            #     print(pile_idx, 'before', before, 'after', len(next_piles), 'took', time.time()-start_time)

            s += len(next_piles)
            # Add new combinations
            for next_pile in next_piles:
                new_combination = prev_piles + [next_pile]
                new_combinations.append(new_combination)

            if pile_idx==0:
                # Print each combination to CSV file
                with open('piles.csv', 'w') as f:
                    for combination in new_combinations:
                        # Convert each pile to a string of 0s and 1s
                        pile_str = ','.join(''.join(str(x) for x in pile) for pile in combination)
                        f.write(pile_str + '\n')
                break
        nums[int(pile_idx+1)] = s
        pile_combinations = new_combinations
    return nums, pile_combinations

def main():
    parser = argparse.ArgumentParser(description='Process pile and cube counts')
    parser.add_argument('n_piles', type=int, help='Number of piles')
    parser.add_argument('n_cubes', type=int, help='Number of cubes')
    args = parser.parse_args()

    n_piles = args.n_piles
    n_cubes = args.n_cubes

    # diophantine = read_diophantine()

    start = time.time()
    nums, pile_combinations = solve(n_piles, n_cubes)
    print(time.time())
    print(f'time taken: {time.time()-start:.2f}')
    # Print all solutions
    # for combination in pile_combinations:
        # ok = print_piles(combination, n_piles, n_cubes)
        # if not ok:
        #     import sys 
        #     sys.exit(1)

    print(" ")

    print("\nPile\tSolutions")
    print("-" * 20)
    for pile_num, solutions in nums.items():
        print(f"{pile_num}\t{solutions}")

# def test_diophantine():
#     # diophantine = read_diophantine()

#     # 6^3+8^3+10^3 are all set and 12^3 is not set, and we could have set 12
#     piles = [[0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]]
#     disallowed = 0

#     # 6^3+8^3+10^3 are all set and 12^3 is not set, and but we could not have set 12
#     piles = [[0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]]
#     disallowed = 1<<11
#     assert len(filter_diophantine(piles, disallowed, diophantine))==1

#     # 8^3+10^3 are all set and 12^3 is not set, and we could have set 12 e.g. this filter just misses
#     piles = [[0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]]
#     disallowed = 0
#     assert len(filter_diophantine(piles, disallowed, diophantine))==1

# if __name__=="__main__":
#     main()
#     # test_diophantine()