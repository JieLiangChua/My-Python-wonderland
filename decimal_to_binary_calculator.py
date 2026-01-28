print("Decimal to Binary Calculator")
binary_output = ""
number_1 = int(input("Enter an integer: "))
while number_1 > 0:
    remainder_1 = number_1 % 2
    binary_output = str(remainder_1) + binary_output
    number_1 = number_1 // 2
print("Binary form: " + binary_output)