import math

def log_scale_value(angle, v_min, v_max, angle_min, angle_max):
    """
    Calculates the logarithmic scale value from an angle.

    angle     — current angle (in degrees)
    v_min     — minimum scale value
    v_max     — maximum scale value
    angle_min — angle for v_min (in degrees)
    angle_max — angle for v_max (in degrees)

    Returns the scale value.
    """

    if v_min <= 0 or v_max <= 0:
        raise ValueError("Scale values must be > 0 for logarithm")

    if angle_min == angle_max:
        raise ValueError("angle_min and angle_max must not be equal")

    # Normalized position by angle
    t = (angle - angle_min) / (angle_max - angle_min)

    # Inverse logarithmic transformation
    log_v = math.log10(v_min) + t * (math.log10(v_max) - math.log10(v_min))
    v = 10 ** log_v

    return v

def log_scale_angle(v, v_min, v_max, angle_min, angle_max):
    """
    Calculates the angle for a logarithmic scale.

    v         — target value
    v_min     — minimum scale value
    v_max     — maximum scale value
    angle_min — angle for v_min (in degrees)
    angle_max — angle for v_max (in degrees)

    Returns the angle in degrees.
    """

    if v <= 0 or v_min <= 0 or v_max <= 0:
        raise ValueError("All scale values must be > 0 for logarithm")

    if not (v_min <= v <= v_max):
        raise ValueError("Value v must be within the scale range")

    # Normalized logarithmic position
    t = (math.log10(v) - math.log10(v_min)) / (math.log10(v_max) - math.log10(v_min))

    # Linear angle interpolation
    angle = angle_min + t * (angle_max - angle_min)

    return angle

    return angle

a_min = 190
a_max = 315
a_step = 1
s_min = 1000
s_max = 5000

# print(log_scale_angle(1000, s_min, s_max, a_min, a_max))
# print(log_scale_angle(2000, s_min, s_max, a_min, a_max))
# print(log_scale_angle(5000, s_min, s_max, a_min, a_max))

# for v in range(1000, 6000, 100):
#     angle = log_scale_angle(v, s_min, s_max, a_min, a_max)
#     print(f"Value: {v}, Angle: {angle:.2f} degrees")

precalc_a = []
precalc_v = []

for angle in range(a_min, a_max + 1, a_step):
    value = log_scale_value(angle, s_min, s_max, a_min, a_max)
    precalc_a.append(angle)
    precalc_v.append(round(value))
    print(f"Angle: {angle} degrees, Value: {value:.0f}")

print("# Calculated Logarithmic Scale Values:")
print(f"# Angle: {a_min}° to {a_max}°, step {a_step}°")
print(f"# Values: {s_min} to {s_max}")
print(f"var log_a = [{','.join(map(str, precalc_a))}]")
print(f"var log_v = [{','.join(map(str, precalc_v))}]")
