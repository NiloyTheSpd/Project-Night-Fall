import React, { useState, useEffect, useRef, useCallback } from 'react';
import { 
  Power, 
  Wifi, 
  Battery, 
  BatteryCharging, 
  Activity, 
  AlertTriangle, 
  Map as MapIcon, 
  Settings, 
  Video, 
  Crosshair, 
  Target, 
  Thermometer, 
  Wind, 
  Cpu, 
  ArrowUp, 
  ArrowDown, 
  ArrowLeft, 
  ArrowRight, 
  RotateCw, 
  StopCircle,
  Terminal,
  Layers,
  Zap
} from 'lucide-react';

// --- Utility Components ---

const Card = ({ children, className = "", title, icon: Icon, color = "text-blue-400" }) => (
  <div className={`bg-gray-900 border border-gray-800 rounded-lg overflow-hidden shadow-lg ${className}`}>
    {(title || Icon) && (
      <div className="bg-gray-800/50 px-4 py-2 border-b border-gray-800 flex items-center justify-between">
        <div className="flex items-center gap-2">
          {Icon && <Icon size={16} className={color} />}
          <span className="text-gray-300 font-mono text-sm uppercase tracking-wider">{title}</span>
        </div>
      </div>
    )}
    <div className="p-4">{children}</div>
  </div>
);

const ProgressBar = ({ value, max = 100, color = "bg-blue-500", height = "h-2" }) => (
  <div className={`w-full bg-gray-700 rounded-full ${height} overflow-hidden`}>
    <div 
      className={`h-full transition-all duration-300 ${color}`} 
      style={{ width: `${Math.min(100, Math.max(0, (value / max) * 100))}%` }}
    />
  </div>
);

const Sparkline = ({ data, color = "stroke-blue-500", height = 40 }) => {
  if (!data || data.length < 2) return null;
  const max = Math.max(...data, 1);
  const min = Math.min(...data);
  const range = max - min || 1;
  
  const points = data.map((d, i) => {
    const x = (i / (data.length - 1)) * 100;
    const y = 100 - ((d - min) / range) * 100;
    return `${x},${y}`;
  }).join(' ');

  return (
    <svg className="w-full overflow-visible" height={height} viewBox="0 0 100 100" preserveAspectRatio="none">
      <polyline points={points} fill="none" strokeWidth="2" className={color} vectorEffect="non-scaling-stroke" />
    </svg>
  );
};

// --- Main Application ---

export default function RobotDashboard() {
  // --- State Management ---
  
  // System Status
  const [connection, setConnection] = useState({ status: 'connected', ping: 24, lastHeartbeat: Date.now() });
  const [battery, setBattery] = useState({ voltage: 14.2, percentage: 87, charging: false });
  const [robotState, setRobotState] = useState('IDLE'); // IDLE, MANUAL, AUTONOMOUS, ERROR
  const [mode, setMode] = useState('MANUAL');
  
  // Sensors
  const [sensors, setSensors] = useState({
    distanceFront: 120,
    distanceRear: 80,
    gas: 150,
    temp: 34.5,
    humidity: 45
  });
  
  // Navigation
  const [odometry, setOdometry] = useState({ x: 0, y: 0, heading: 0, speed: 0 });
  const [motors, setMotors] = useState({ left: 0, right: 0 });
  
  // Mission
  const [mission, setMission] = useState({
    active: false,
    currentWaypoint: 1,
    totalWaypoints: 5,
    progress: 20
  });

  // Logs & Alerts
  const [logs, setLogs] = useState([
    { id: 1, time: new Date().toLocaleTimeString(), type: 'info', msg: 'Dashboard ready for connection' },
  ]);
  
  // History for graphs
  const [history, setHistory] = useState({
    battery: [],
    speed: [],
    gas: []
  });

  // ML Detection Mock
  const [detection, setDetection] = useState({
    label: 'None',
    confidence: 0,
    box: null
  });

  // --- Real ESP32 Integration ---
  const ESP32_IP = '192.168.4.1'; // Front ESP32 WiFi AP
  const wsRef = useRef(null);

  useEffect(() => {
    // WebSocket connection to front ESP32
    const connectWebSocket = () => {
      const ws = new WebSocket(`ws://${ESP32_IP}/ws`);
      wsRef.current = ws;
      
      ws.onopen = () => {
        handleLog('info', '✓ Connected to Robot WebSocket');
        setConnection(prev => ({ ...prev, status: 'connected' }));
      };
      
      ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          const now = Date.now();
          
          // Update sensors
          setSensors({
            distanceFront: data.front_distance || 0,
            distanceRear: data.rear_distance || 0,
            gas: data.gas || 0,
            temp: 34.5, // Add real temp sensor if available
            humidity: 45
          });
          
          // Update state
          const states = ['INIT', 'IDLE', 'AUTONOMOUS', 'MANUAL', 'EMERGENCY', 'CLIMBING', 'TURNING', 'AVOIDING'];
          setRobotState(states[data.state] || 'IDLE');
          setMode(data.autonomous ? 'AUTONOMOUS' : 'MANUAL');
          
          // Update connection ping
          setConnection({ 
            status: 'connected', 
            ping: data.uptime_ms ? (now - data.uptime_ms) % 1000 : 24,
            lastHeartbeat: now
          });

          // Update history for graphs
          setHistory(prev => ({
            battery: [...prev.battery.slice(-20), battery.percentage],
            speed: [...prev.speed.slice(-20), odometry.speed],
            gas: [...prev.gas.slice(-20), data.gas || 0]
          }));
          
        } catch (err) {
          console.error('WebSocket parse error:', err);
        }
      };

      ws.onerror = (err) => {
        handleLog('critical', 'WebSocket error - check connection');
        setConnection({ status: 'disconnected', ping: 0, lastHeartbeat: Date.now() });
      };
      
      ws.onclose = () => {
        handleLog('warning', 'WebSocket closed - reconnecting...');
        setConnection({ status: 'disconnected', ping: 0, lastHeartbeat: Date.now() });
        setTimeout(connectWebSocket, 2000);
      };
    };

    connectWebSocket();
    
    return () => {
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, [battery.percentage, odometry.speed]);

  // --- Handlers ---

  const handleLog = (type, msg) => {
    const time = new Date().toLocaleTimeString('en-US', { hour12: false });
    setLogs(prev => [{ id: Date.now(), time, type, msg }, ...prev].slice(0, 50));
  };

  const emergencyStop = useCallback(() => {
    sendCommand('estop');
    setRobotState('EMERGENCY');
    setMotors({ left: 0, right: 0 });
    setOdometry(prev => ({ ...prev, speed: 0 }));
    handleLog('critical', '🛑 EMERGENCY STOP TRIGGERED');
  }, [sendCommand]);

  const toggleMode = () => {
    const newMode = mode === 'MANUAL' ? 'AUTONOMOUS' : 'MANUAL';
    sendCommand(newMode === 'AUTONOMOUS' ? 'autonomous_on' : 'autonomous_off');
    handleLog('info', `Switched to ${newMode} mode`);
  };

  const sendCommand = useCallback((cmd) => {
    // Send via HTTP POST API
    fetch(`http://${ESP32_IP}/api/command`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ cmd })
    }).catch(err => {
      handleLog('critical', `Failed to send command: ${cmd}`);
      console.error(err);
    });
  }, []);

  const handleControl = useCallback((action) => {
    if (robotState === 'EMERGENCY' || mode === 'AUTONOMOUS') return;
    
    const cmdMap = {
      'UP': 'forward',
      'DOWN': 'backward',
      'LEFT': 'left',
      'RIGHT': 'right',
      'STOP': 'stop'
    };
    
    const cmd = cmdMap[action];
    if (cmd) {
      sendCommand(cmd);
      handleLog('info', `Command sent: ${cmd}`);
    }
  }, [robotState, mode, sendCommand]);

  // Keyboard Listeners
  useEffect(() => {
    const handleKeyDown = (e) => {
      if (e.repeat) return;
      if (e.key === ' ') { e.preventDefault(); emergencyStop(); }
      if (e.key === 'w' || e.key === 'ArrowUp') handleControl('UP');
      if (e.key === 's' || e.key === 'ArrowDown') handleControl('DOWN');
      if (e.key === 'a' || e.key === 'ArrowLeft') handleControl('LEFT');
      if (e.key === 'd' || e.key === 'ArrowRight') handleControl('RIGHT');
    };
    
    const handleKeyUp = (e) => {
      if (['w', 's', 'ArrowUp', 'ArrowDown'].includes(e.key)) handleControl('STOP');
      if (['a', 'd', 'ArrowLeft', 'ArrowRight'].includes(e.key)) handleControl('STOP');
    };

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, [handleControl, emergencyStop]);


  // --- Helper Functions for UI ---

  const getBatteryColor = (level) => {
    if (level > 50) return "text-emerald-400";
    if (level > 20) return "text-yellow-400";
    return "text-red-500 animate-pulse";
  };

  const getDistanceColor = (dist) => {
    if (dist > 50) return "bg-emerald-500";
    if (dist > 20) return "bg-yellow-500";
    return "bg-red-500";
  };

  const getStateColor = (state) => {
    switch(state) {
      case 'IDLE': return 'bg-gray-600';
      case 'MANUAL': return 'bg-blue-600';
      case 'AUTONOMOUS': return 'bg-purple-600';
      case 'EMERGENCY': return 'bg-red-600 animate-pulse';
      default: return 'bg-gray-600';
    }
  };

  // --- Main Render ---

  return (
    <div className="min-h-screen bg-black text-gray-200 font-sans selection:bg-blue-500/30 overflow-hidden flex flex-col">
      
      {/* 1. TOP BAR */}
      <header className="h-14 bg-gray-900 border-b border-gray-800 flex items-center justify-between px-4 shrink-0 z-50">
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-2 text-blue-400">
            <Cpu className="animate-spin-slow" />
            <span className="font-bold text-lg tracking-wider">ROBO-OS v2.4</span>
          </div>
          <div className="h-6 w-px bg-gray-700 mx-2 hidden md:block"></div>
          <div className={`px-3 py-1 rounded-full text-xs font-bold tracking-wider flex items-center gap-2 ${getStateColor(robotState)} text-white`}>
            <Activity size={14} />
            {robotState}
          </div>
        </div>

        <div className="flex items-center gap-6">
           {/* Connection */}
          <div className="flex items-center gap-2 text-sm font-mono">
             <div className={`w-2 h-2 rounded-full ${connection.status === 'connected' ? 'bg-emerald-500' : 'bg-red-500 animate-ping'}`} />
             <span className={connection.status === 'connected' ? 'text-emerald-400' : 'text-red-400'}>
               {connection.ping}ms
             </span>
             <Wifi size={16} className="text-gray-500" />
          </div>

          {/* Battery */}
          <div className="flex items-center gap-3 bg-gray-800 px-3 py-1 rounded-lg border border-gray-700">
            <div className="flex flex-col items-end leading-none">
              <span className={`text-sm font-bold font-mono ${getBatteryColor(battery.percentage)}`}>
                {battery.voltage.toFixed(1)}V
              </span>
              <span className="text-[10px] text-gray-400">{Math.floor(battery.percentage)}%</span>
            </div>
            {battery.charging ? <BatteryCharging size={20} className="text-yellow-400" /> : <Battery size={20} className={getBatteryColor(battery.percentage)} />}
          </div>

          {/* E-STOP Header Button (Mobile/Tablet Access) */}
          <button 
            onClick={emergencyStop}
            className="bg-red-600 hover:bg-red-700 text-white p-2 rounded-md font-bold transition-colors md:hidden"
          >
            STOP
          </button>
        </div>
      </header>

      {/* MAIN GRID LAYOUT */}
      <main className="flex-1 overflow-auto p-2 md:p-4 grid grid-cols-1 md:grid-cols-12 gap-4">
        
        {/* LEFT COLUMN: Camera & Controls (40% on Desktop) */}
        <div className="md:col-span-5 lg:col-span-5 flex flex-col gap-4">
          
          {/* CAMERA FEED - CRITICAL */}
          <Card className="relative group min-h-[300px] flex flex-col" title="Live Feed" icon={Video} color="text-red-400">
            <div className="relative flex-1 bg-gray-950 rounded border border-gray-800 overflow-hidden flex items-center justify-center">
              {/* Live ESP32-CAM Feed */}
              <img 
                src={`http://${ESP32_IP}:81/stream`} 
                alt="ESP32-CAM Live Stream"
                className="w-full h-full object-contain"
                onError={(e) => {
                  e.target.style.display = 'none';
                  handleLog('warning', 'Camera stream unavailable');
                }}
                onLoad={(e) => {
                  e.target.style.display = 'block';
                }}
              />
              
              {/* Crosshair Overlay */}
              <div className="absolute inset-0 flex items-center justify-center pointer-events-none opacity-30">
                <Crosshair className="text-white/50 w-16 h-16" />
              </div>

              {/* ML Detection Bounding Box */}
              {detection.box && (
                <div 
                  className="absolute border-2 border-yellow-400 bg-yellow-400/10 transition-all duration-300"
                  style={{ 
                    left: `${detection.box.x}%`, 
                    top: `${detection.box.y}%`, 
                    width: `${detection.box.w}%`, 
                    height: `${detection.box.h}%` 
                  }}
                >
                  <div className="absolute -top-6 left-0 bg-yellow-400 text-black text-xs font-bold px-1 py-0.5 rounded-t">
                    {detection.label} {detection.confidence}%
                  </div>
                </div>
              )}

              {/* Status Overlays */}
              <div className="absolute top-4 left-4 flex flex-col gap-1">
                <div className="flex items-center gap-2 bg-black/60 px-2 py-1 rounded text-red-500 text-xs font-bold animate-pulse">
                  <div className="w-2 h-2 rounded-full bg-red-500" /> REC
                </div>
                <div className="text-xs text-gray-400 font-mono">1280x720 @ 30fps</div>
              </div>
            </div>
          </Card>

          {/* CONTROLS - CRITICAL */}
          <div className="grid grid-cols-2 gap-4">
            <Card className="col-span-2 md:col-span-1" title="Movement" icon={Target}>
              <div className="flex flex-col items-center gap-2 py-2">
                <button 
                  onMouseDown={() => handleControl('UP')} 
                  onMouseUp={() => handleControl('STOP')}
                  className="w-12 h-12 bg-gray-800 hover:bg-gray-700 active:bg-blue-600 rounded-lg flex items-center justify-center border border-gray-700 transition-colors"
                >
                  <ArrowUp size={24} />
                </button>
                <div className="flex gap-2">
                  <button 
                    onMouseDown={() => handleControl('LEFT')} 
                    onMouseUp={() => handleControl('STOP')}
                    className="w-12 h-12 bg-gray-800 hover:bg-gray-700 active:bg-blue-600 rounded-lg flex items-center justify-center border border-gray-700 transition-colors"
                  >
                    <ArrowLeft size={24} />
                  </button>
                  <button 
                    onMouseDown={() => handleControl('DOWN')} 
                    onMouseUp={() => handleControl('STOP')}
                    className="w-12 h-12 bg-gray-800 hover:bg-gray-700 active:bg-blue-600 rounded-lg flex items-center justify-center border border-gray-700 transition-colors"
                  >
                    <ArrowDown size={24} />
                  </button>
                  <button 
                    onMouseDown={() => handleControl('RIGHT')} 
                    onMouseUp={() => handleControl('STOP')}
                    className="w-12 h-12 bg-gray-800 hover:bg-gray-700 active:bg-blue-600 rounded-lg flex items-center justify-center border border-gray-700 transition-colors"
                  >
                    <ArrowRight size={24} />
                  </button>
                </div>
                <div className="mt-2 text-xs text-gray-500 font-mono">WASD / ARROWS</div>
              </div>
            </Card>

            <Card className="col-span-2 md:col-span-1 flex flex-col justify-between" title="Actions" icon={Settings}>
              <div className="flex flex-col gap-3">
                <button 
                  onClick={toggleMode}
                  className={`flex items-center justify-center gap-2 py-3 px-4 rounded-lg font-bold text-sm transition-all ${
                    mode === 'AUTONOMOUS' 
                    ? 'bg-purple-600 hover:bg-purple-700 text-white shadow-[0_0_15px_rgba(147,51,234,0.3)]' 
                    : 'bg-gray-700 hover:bg-gray-600 text-gray-200'
                  }`}
                >
                   {mode === 'AUTONOMOUS' ? <Zap size={16} /> : <Terminal size={16} />}
                   {mode === 'AUTONOMOUS' ? 'AUTO MODE' : 'MANUAL MODE'}
                </button>
                
                <button 
                  onClick={emergencyStop}
                  className="bg-red-600 hover:bg-red-700 active:scale-95 text-white py-4 px-4 rounded-lg font-bold text-lg flex items-center justify-center gap-2 shadow-[0_0_20px_rgba(220,38,38,0.4)] transition-all border border-red-500"
                >
                  <Power size={24} /> E-STOP
                </button>
              </div>
            </Card>
          </div>
        </div>

        {/* MIDDLE COLUMN: Sensors & Telemetry (35% on Desktop) */}
        <div className="md:col-span-4 lg:col-span-4 flex flex-col gap-4">
          
          {/* SENSORS - HIGH PRIORITY */}
          <Card title="Environment Sensors" icon={Layers} color="text-yellow-400">
            <div className="space-y-4">
              
              {/* Distance Sensors */}
              <div className="space-y-2">
                <div className="flex justify-between text-xs text-gray-400 uppercase font-mono">
                  <span>Front Distance</span>
                  <span>{sensors.distanceFront.toFixed(0)} CM</span>
                </div>
                <div className="flex items-center gap-2">
                  <div className="flex-1 bg-gray-800 rounded-full h-3 overflow-hidden">
                    <div 
                      className={`h-full transition-all duration-100 ${getDistanceColor(sensors.distanceFront)}`} 
                      style={{ width: `${Math.min(100, (sensors.distanceFront / 200) * 100)}%` }}
                    />
                  </div>
                  <AlertTriangle size={14} className={sensors.distanceFront < 20 ? "text-red-500 opacity-100" : "opacity-0"} />
                </div>
              </div>

              <div className="space-y-2">
                 <div className="flex justify-between text-xs text-gray-400 uppercase font-mono">
                  <span>Rear Distance</span>
                  <span>{sensors.distanceRear.toFixed(0)} CM</span>
                </div>
                <div className="flex items-center gap-2">
                  <div className="flex-1 bg-gray-800 rounded-full h-3 overflow-hidden">
                    <div 
                      className={`h-full transition-all duration-100 ${getDistanceColor(sensors.distanceRear)}`} 
                      style={{ width: `${Math.min(100, (sensors.distanceRear / 200) * 100)}%` }}
                    />
                  </div>
                   <AlertTriangle size={14} className={sensors.distanceRear < 20 ? "text-red-500 opacity-100" : "opacity-0"} />
                </div>
              </div>

              <div className="h-px bg-gray-800 my-2"></div>

              {/* Atmospheric Sensors */}
              <div className="grid grid-cols-2 gap-4">
                <div className="bg-gray-800/50 p-2 rounded border border-gray-700/50">
                  <div className="flex items-center gap-2 text-xs text-gray-400 mb-1">
                    <Wind size={12} /> GAS LEVEL
                  </div>
                  <div className={`text-xl font-mono font-bold ${sensors.gas > 300 ? 'text-red-400 animate-pulse' : 'text-emerald-400'}`}>
                    {sensors.gas.toFixed(0)}
                    <span className="text-xs text-gray-500 font-normal ml-1">PPM</span>
                  </div>
                </div>
                <div className="bg-gray-800/50 p-2 rounded border border-gray-700/50">
                   <div className="flex items-center gap-2 text-xs text-gray-400 mb-1">
                    <Thermometer size={12} /> TEMP
                  </div>
                  <div className="text-xl font-mono font-bold text-blue-400">
                    {sensors.temp.toFixed(1)}°
                  </div>
                </div>
              </div>
            </div>
          </Card>

          {/* TELEMETRY GRAPHS - LOW PRIORITY */}
          <Card title="Telemetry" icon={Activity} color="text-purple-400" className="flex-1 min-h-[200px]">
             <div className="flex flex-col gap-6 h-full justify-center">
               <div>
                  <div className="flex justify-between text-xs text-gray-500 mb-1">
                    <span>BATTERY DRAIN</span>
                    <span>{battery.voltage.toFixed(1)}V</span>
                  </div>
                  <Sparkline data={history.battery} color="stroke-emerald-500" />
               </div>
                <div>
                  <div className="flex justify-between text-xs text-gray-500 mb-1">
                    <span>GAS SENSOR HISTORY</span>
                    <span>AVG: {Math.floor(sensors.gas)}</span>
                  </div>
                  <Sparkline data={history.gas} color="stroke-yellow-500" />
               </div>
             </div>
          </Card>
        </div>

        {/* RIGHT COLUMN: Mission & Logs (25% on Desktop) */}
        <div className="md:col-span-3 lg:col-span-3 flex flex-col gap-4">
          
          {/* MINI MAP - MEDIUM PRIORITY */}
          <Card title="Localization" icon={MapIcon} color="text-blue-400">
            <div className="aspect-square bg-gray-950 rounded border border-gray-800 relative overflow-hidden">
               {/* Grid */}
               <div className="absolute inset-0 opacity-20" style={{ backgroundImage: 'radial-gradient(#444 1px, transparent 1px)', backgroundSize: '20px 20px' }}></div>
               
               {/* Waypoints */}
               <div className="absolute top-[30%] left-[60%] w-3 h-3 bg-blue-500/50 rounded-full border border-blue-400"></div>
               <div className="absolute top-[70%] left-[20%] w-3 h-3 bg-blue-500/50 rounded-full border border-blue-400"></div>

               {/* Robot Marker - Animated */}
               <div 
                 className="absolute w-0 h-0 border-l-[8px] border-l-transparent border-r-[8px] border-r-transparent border-b-[16px] border-b-emerald-500 transition-all duration-300"
                 style={{ 
                   top: '50%', 
                   left: '50%', 
                   transform: `translate(-50%, -50%) rotate(${odometry.heading}deg)`
                 }}
               ></div>
               
               {/* Heading Text */}
               <div className="absolute bottom-2 right-2 font-mono text-xs text-gray-500">
                 H: {odometry.heading}°
               </div>
            </div>
          </Card>

          {/* MISSION STATUS - MEDIUM PRIORITY */}
          <Card title="Mission Status" icon={Target} color="text-green-400">
             <div className="space-y-3">
               <div className="flex justify-between items-end">
                 <span className="text-sm text-gray-300">Waypoint {mission.currentWaypoint}/{mission.totalWaypoints}</span>
                 <span className="text-xs text-blue-400 font-mono">RUNNING</span>
               </div>
               <ProgressBar value={mission.progress} color="bg-blue-500" />
               <div className="text-xs text-gray-500 flex justify-between">
                 <span>Dist: 245m</span>
                 <span>ETA: 4m 12s</span>
               </div>
             </div>
          </Card>

          {/* ALERTS / LOGS - MEDIUM PRIORITY */}
          <Card title="System Logs" icon={Terminal} color="text-gray-400" className="flex-1 overflow-hidden flex flex-col">
            <div className="flex-1 overflow-y-auto space-y-2 pr-2 font-mono text-xs h-[200px] scrollbar-thin scrollbar-thumb-gray-700 scrollbar-track-transparent">
              {logs.map((log) => (
                <div key={log.id} className="flex gap-2 border-l-2 border-gray-700 pl-2 py-0.5">
                   <span className="text-gray-500 shrink-0">[{log.time}]</span>
                   <span className={`${log.type === 'critical' ? 'text-red-400 font-bold' : log.type === 'warning' ? 'text-yellow-400' : 'text-gray-300'}`}>
                     {log.msg}
                   </span>
                </div>
              ))}
            </div>
          </Card>

        </div>
      </main>
    </div>
  );
}