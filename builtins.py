uec_grade = float(input("Enter your UEC average: "))
hardware_passion = int(input("How much hardware passion? (1-10)"))
github_commits = int(input("How many commits?"))

if uec_grade<=2.0 and hardware_passion>=9:
    print("STATUS: Top-tier Candidate. Your hardware intuition and academic base are a perfect match!")
elif uec_grade>2.0 and github_commits>50:
    print("STATUS: Resilience Detected. Using portfolio strength and consistency to outperform the grades!")
else:
    print("STATUS: Building Phase. Keep pushing your limits. Persistence is the key to CS!")