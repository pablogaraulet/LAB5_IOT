from flask import Flask, request, jsonify
import math

app = Flask(__name__)

step_count = 0
jump_count = 0
last_detected = ""

@app.route('/sensor', methods=['POST'])
def sensor_data():
    global step_count, jump_count, last_detected
    data = request.get_json()

    ax = data.get('ax', 0)
    ay = data.get('ay', 0)
    az = data.get('az', 0)

    mag = math.sqrt(ax**2 + ay**2 + az**2)
    detected = ""

    if mag > 1.35 and az > 1.2:
        jump_count += 1
        detected = "JUMP"
    elif mag > 1.0 and az <= 1.2:
        step_count += 1
        detected = "STEP"

    last_detected = detected
    print(f"Mag: {mag:.2f}, Z: {az:.2f} -> {detected}")

    return jsonify({
        "steps": step_count,
        "jumps": jump_count,
        "detected": detected
    })

@app.route('/reset', methods=['POST'])
def reset_counters():
    global step_count, jump_count, last_detected
    step_count = 0
    jump_count = 0
    last_detected = ""
    print(">>> Counters reset by ESP32")
    return jsonify({"status": "reset"})

@app.route('/stats', methods=['GET'])
def stats():
    return jsonify({
        "steps": step_count,
        "jumps": jump_count
    })

@app.route('/', methods=['GET'])
def index():
    return f"""
    <h2>Sensor Summary</h2>
    <p>Last Detected: <strong>{last_detected}</strong></p>
    <p>Total Steps: <strong>{step_count}</strong></p>
    <p>Total Jumps: <strong>{jump_count}</strong></p>
    """

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
