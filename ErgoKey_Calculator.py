lines_of_code = int(input("How many lines?\n"))
total_keystrokes = lines_of_code * 50
if total_keystrokes > 10000:
  print("CRITICAL: Total keystrokes exceeded 10,000. Please ensure you are using a keyboard with 1.5mm key travel to prevent joint fatigue. Take a 15-minute break immediately!")
elif total_keystrokes > 5000:
  print(f"WARNING: Heavy coding session detected. Total keystrokes: {total_keystrokes}. Consider ergonomic adjustments and stretch your fingers every 30 minutes.")
else:
  print("STATUS: Workload is within safe limits. Your fingers are in good shape. Enjoy your coding journey!")
  