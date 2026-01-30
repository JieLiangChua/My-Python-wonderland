print("Decimal to N-ary Converter")
table = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
n_decimal_output = ""
number_1 = int(input("Enter an integer: "))
base = int(input("Enter the base (2-36) : "))

if base > 36 or base < 2:
    print("Invalid base. Please reenter.")
else:
    if number_1 == 0:
        n_decimal_output = "0"
        
    while number_1 > 0:
        remainder_1 = number_1 % base
        char = table[remainder_1] 
        n_decimal_output = char + n_decimal_output
        number_1 = number_1 // base

    if base == 2:
        base_name = "Binary"
    elif base == 8:
        base_name = "Octal"
    elif base == 16:
        base_name = "Hexadecimal"
    else:
        base_name = f"Base-{base}"

    print(f"{base_name} form: {n_decimal_output}")