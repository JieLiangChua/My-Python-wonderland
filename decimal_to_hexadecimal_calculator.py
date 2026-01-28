print("Decimal to Hexadecimal Calculator")
table = "0123456789ABCDEF"  # Wardrobe
hexadecimal_output = ""
number_1 = int(input("Enter an integer: "))

while number_1 > 0:
    remainder_1 = number_1 % 16
    char = table[remainder_1] 
    hexadecimal_output = char + hexadecimal_output
    number_1 = number_1 // 16

print("Hexadecimal form: " + hexadecimal_output)