import piles
import argparse
import json

    # assigned_piles = init_distribution();
    # auto start_pos = init_pos(assigned_piles);

    # assigned_remaining = init_remaining(assigned_piles);
sums = [1, 9, 36, 100, 225, 441, 784, 1296, 2025, 3025, 4356, 6084, 8281, 11025, 14400, 18496, 23409, 29241, 36100, 44100, 53361, 64009, 76176, 90000, 105625, 123201, 142884, 164836, 189225, 216225, 246016, 278784, 314721, 354025, 396900, 443556, 494209, 549081, 608400, 672400, 741321, 815409, 894916, 980100, 1071225, 1168561, 1272384, 1382976, 1500625, 1625625, 1758276, 1898884, 2047761, 2205225, 2371600, 2547216, 2732409, 2927521, 3132900, 3348900, 3575881, 3814209, 4064256, 4326400, 4601025, 4888521, 5189284, 5503716, 5832225, 6175225, 6533136, 6906384, 7295401, 7700625, 8122500, 8561476, 9018009, 9492561, 9985600, 10497600, 11029041, 11580409, 12152196, 12744900, 13359025, 13995081, 14653584, 15335056, 16040025, 16769025, 17522596, 18301284, 19105641, 19936225, 20793600, 21678336, 22591009, 23532201, 24502500, 25502500]
cubes = [1, 8, 27, 64, 125, 216, 343, 512, 729, 1000, 1331, 1728, 2197, 2744, 3375, 4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167, 13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791, 32768, 35937, 39304, 42875, 46656, 50653, 54872, 59319, 64000, 68921, 74088, 79507, 85184, 91125, 97336, 103823, 110592, 117649, 125000, 132651, 140608, 148877, 157464, 166375, 175616, 185193, 195112, 205379, 216000, 226981, 238328, 250047, 262144, 274625, 287496, 300763, 314432, 328509, 343000, 357911, 373248, 389017, 405224, 421875, 438976, 456533, 474552, 493039, 512000, 531441, 551368, 571787, 592704, 614125, 636056, 658503, 681472, 704969, 729000, 753571, 778688, 804357, 830584, 857375, 884736, 912673, 941192, 970299, 1000000]

parser = argparse.ArgumentParser(description='Process pile and cube counts')
parser.add_argument('n_piles', type=int, help='Number of piles')
parser.add_argument('n_cubes', type=int, help='Number of cubes')
args = parser.parse_args()

n_piles = args.n_piles
n_cubes = args.n_cubes

assigned_piles = piles.init_distribution(n_piles, n_cubes)
start_pos = piles.init_pos(assigned_piles)


assigned_remaining = piles.init_remaining(assigned_piles, n_piles)
disallowed = [0] * n_cubes
# Initialize list to store all pile combinations
pile_combinations = [[]]

# Iterate through all piles
nums = {}
for pile_idx in range(n_piles):
    s=0
    new_combinations = []
    
    # For each existing combination of piles
    for prev_piles in pile_combinations:
        # Calculate target and remaining for next pile
        if pile_idx == 0:
            # First pile uses initial values
            target = assigned_remaining[0]
            remaining = sums[n_cubes-1]
            assigned_pile = assigned_piles[0]
            curr_disallowed = disallowed
        else:
            # Subsequent piles use values based on previous piles
            target = assigned_remaining[pile_idx]
            curr_disallowed = [0] * n_cubes
            for prev_pile in prev_piles:
                for i in range(len(prev_pile)):
                    if prev_pile[i]:
                        curr_disallowed[i] = 1
            remaining = piles.calc_remaining(curr_disallowed, n_cubes)
            assigned_pile = assigned_piles[pile_idx]
            
        # Generate next pile
        next_piles = piles.make_pile(target, remaining, start_pos, assigned_pile, curr_disallowed)
        s+=len(next_piles)
        # Add new combinations
        for next_pile in next_piles:
            new_combination = prev_piles + [next_pile]
            new_combinations.append(new_combination)
    nums[int(pile_idx+1)]=s
    pile_combinations = new_combinations
print(json.dumps(nums))

# Print final combinations
# for combination in pile_combinations:
#     for pile in combination:
#         # Print cube numbers that are used (have value 1) in this pile
#         # used_cubes = [str(i+1) for i in range(n_cubes-1)]
#         s= sum((used*int(i+1))**3 for i,used in enumerate(pile))
#         c = [i+1 for i, c in enumerate(pile) if c==1]
#         print(f"{''.join(map(str, pile))} : {','.join(map(str,c))} - {s}")
#     print(" ")

    # print(' '.join(''.join(map(str, pile)) for pile in combination))
