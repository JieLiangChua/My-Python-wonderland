my_budget = int(input("How many budget in Ringgit Malaysia?\n"))
if my_budget > 5000:
    print(f"RM {my_budget} is a great budget! You can get a 2.5K 16:10 laptop...")
elif my_budget >=3000:
    print("Good. You can get a solid 16:10 productivity machine.")
else:
    print("Warning: It might be hard to find a 2.5K screen at this price. Save more for your eyes!")
