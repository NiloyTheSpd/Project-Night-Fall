import React, { useState, useEffect, useRef } from 'react';
import { 
  Power, Wifi, Activity, AlertTriangle, Settings, Video, Crosshair, Target, 
  Wind, Cpu, ArrowUp, ArrowDown, ArrowLeft, ArrowRight, Zap, Terminal, 
  Layers, Radio, Gauge, AlertCircle, Clock
} from 'lucide-react';
import { useNightfallWS } from './hooks/useNightfallWS';

// --- Utility Components ---

const Card = ({ children, className = "", title, icon: Icon, color = "text-blue-400" }) => (
  <div className={`bg-gray-900 border border-gray-800 rounded-xl overflow-hidden shadow-lg flex flex-col ${className}`}>
    {(title || Icon) && (
      <div className="bg-gray-800/30 px-4 py-3 border-b border-gray-800 flex items-center justify-between shrink-0">
        <div className="flex items-center gap-2">
          {Icon && <Icon size={18} className={color} />}
          <span className="text-gray-200 font-bold tracking-wide text-sm">{title}</span>
        </div>
      </div>
    )}
    <div className="p-4 flex-1">{children}</div>
  </div>
);

const Sparkline = ({ data = [], color = "stroke-blue-500", height = 40 }) => {
  if (!data || data.length < 2) return <div style={{ height }} className="w-full bg-gray-800/20 rounded" />;
  const max = Math.max(...data);
  const min = Math.min(...data);
  const range = max - min || 1;
  const points = data.map((d, i) => {
    const x = (i / (data.length - 1)) * 100;
    const y = 100 - ((d - min) / range) * 100;
    return `${x},${y}`;
  }).join(' ');
  return (
    <svg className="w-full overflow-visible" height={height} viewBox="0 0 100 100" preserveAspectRatio="none">
      <polyline points={points} fill="none" className={color} strokeWidth="2" vectorEffect="non-scaling-stroke" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  );
};

// --- Widgets ---

const ConnectionBadge = ({ status, stats, lastPing }) => {
  const styles = {
    connected: { bg: 'bg-emerald-500', text: 'text-emerald-500', label: 'CONNECTED' },
    connecting: { bg: 'bg-yellow-500', text: 'text-yellow-500', label: 'CONNECTING' },
    disconnected: { bg: 'bg-red-500', text: 'text-red-500', label: 'DISCONNECTED' },
    error: { bg: 'bg-red-500', text: 'text-red-500', label: 'ERROR' }
  };
  const s = styles[status] || styles.disconnected;

  return (
    <div className="flex items-center gap-4 bg-gray-900 border border-gray-800 rounded-lg p-3">
      <div className="flex items-center gap-3">
        <div className={`w-3 h-3 rounded-full ${s.bg} ${status === 'connected' ? 'shadow-[0_0_10px_currentColor]' : 'animate-pulse'}`} />
        <div>
           <div className={`font-bold tracking-wider text-sm ${s.text}`}>{s.label}</div>
           <div className="text-[10px] text-gray-500 font-mono flex gap-2">
             <span>{stats.msgRate} msg/s</span>
             <span>•</span>
             <span>{lastPing}ms latency</span>
           </div>
        </div>
      </div>
      {status !== 'connected' && stats.lastError && (
        <div className="ml-auto text-xs text-red-400 font-mono bg-red-900/20 px-2 py-1 rounded">
           {stats.lastError} (x{stats.reconnectAttempts})
        </div>
      )}
    </div>
  );
};

const GasGauge = ({ value, trend }) => {
  let status = value > 2000 ? 'DANGER' : value > 1000 ? 'WARNING' : 'SAFE';
  let color = value > 2000 ? 'text-red-500' : value > 1000 ? 'text-orange-400' : 'text-emerald-400';
  let bg = value > 2000 ? 'bg-red-500/10 border-red-500/50' : value > 1000 ? 'bg-orange-500/10 border-orange-500/50' : 'bg-emerald-500/10 border-emerald-500/50';

  return (
    <div className={`rounded-xl border p-4 ${bg} transition-colors duration-300`}>
      <div className="flex justify-between items-start mb-2">
        <div className="flex items-center gap-2">
          <Wind className={color} size={20} />
          <span className={`text-sm font-bold tracking-wider ${color}`}>AIR QUALITY</span>
        </div>
        <span className={`text-xs font-bold px-2 py-0.5 rounded bg-black/40 ${color}`}>{status}</span>
      </div>
      <div className="flex items-baseline gap-2 mb-2">
        <span className={`text-3xl font-mono font-bold ${color}`}>{value}</span>
        <span className="text-xs text-gray-400 font-mono">PPM</span>
      </div>
      <div className="h-8 w-full">
         <Sparkline data={trend} color={value > 1000 ? "stroke-red-500" : "stroke-emerald-500"} height={32} />
      </div>
    </div>
  );
};

const DistanceWidget = ({ front, rear, trend }) => {
  const getZone = (d) => d < 30 ? 'bg-red-500' : d < 60 ? 'bg-yellow-500' : 'bg-emerald-500';
  return (
    <div className="grid grid-cols-2 gap-3 h-full">
      <div className="bg-gray-800/40 rounded-lg p-3 border border-gray-700 relative overflow-hidden flex flex-col justify-between">
           <div className={`absolute left-0 top-0 bottom-0 w-1 ${getZone(front)}`} />
           <div className="flex justify-between text-xs text-gray-400 font-mono mb-1 pl-2">FRONT</div>
           <div className="text-2xl font-mono font-bold text-white pl-2 z-10">{front.toFixed(1)} <span className="text-xs text-gray-500">cm</span></div>
           {front < 30 && <div className="absolute top-2 right-2 text-[10px] font-bold bg-red-600 text-white px-1.5 rounded animate-pulse z-20">TOO CLOSE</div>}
           <div className="absolute bottom-0 left-0 right-0 h-8 opacity-30 pointer-events-none">
              <Sparkline data={trend} color={front < 30 ? "stroke-red-500" : "stroke-blue-400"} height={32} />
           </div>
      </div>
      <div className="bg-gray-800/40 rounded-lg p-3 border border-gray-700 relative overflow-hidden flex flex-col justify-between">
           <div className={`absolute left-0 top-0 bottom-0 w-1 ${getZone(rear)}`} />
           <div className="flex justify-between text-xs text-gray-400 font-mono mb-1 pl-2">REAR</div>
           <div className="text-2xl font-mono font-bold text-white pl-2">{rear.toFixed(1)} <span className="text-xs text-gray-500">cm</span></div>
           {rear < 30 && <div className="absolute top-2 right-2 text-[10px] font-bold bg-red-600 text-white px-1.5 rounded animate-pulse">TOO CLOSE</div>}
      </div>
    </div>
  );
};

// --- Main App ---

export default function RobotDashboard() {
  const { telemetry, connectionStatus, connectionStats, sendUiCmd, lastPing } = useNightfallWS();
  const { sensors, motors, state, network, server_clients } = telemetry;
  
  // Stats & Trends
  const [history, setHistory] = useState({ gas: [], front: [] });
  useEffect(() => {
    setHistory(h => ({
      gas: [...h.gas.slice(-50), sensors.gas],
      front: [...h.front.slice(-50), sensors.front_dist]
    }));
  }, [sensors.gas, sensors.front_dist]);

  // Optimistic UI
  const [optimisticAuto, setOptimisticAuto] = useState(null);
  const isAuto = optimisticAuto !== null ? optimisticAuto : state.autonomous;
  const isEmerg = state.fsm === 'EMERGENCY';

  // Logs
  const [logs, setLogs] = useState([]);
  const logsEndRef = useRef(null);
  const addLog = (msg, type = 'info') => {
    const colors = { info: 'text-gray-400', warn: 'text-yellow-400', error: 'text-red-400' };
    setLogs(p => [...p.slice(-99), { t: new Date().toLocaleTimeString(), m: msg, c: colors[type] }]);
  };

  // Effects
  useEffect(() => {
    if (state.autonomous === optimisticAuto) setOptimisticAuto(null);
  }, [state.autonomous]);

  useEffect(() => {
    if (isEmerg) addLog('EMERGENCY STATE DETECTED - CONTROLS LOCKED', 'error');
  }, [isEmerg]);
  
  useEffect(() => {
      addLog(`WebSocket ${connectionStatus}`, connectionStatus === 'connected' ? 'info' : 'error');
  }, [connectionStatus]);

  // Handlers
  const handleControl = (cmd) => {
    if (isEmerg || isAuto) return;
    sendUiCmd(cmd);
    // addLog(`CMD: ${cmd}`); // Too noisy for repeated keys
  };

  const toggleAuto = () => {
    if (isEmerg) return;
    const next = !isAuto;
    setOptimisticAuto(next);
    sendUiCmd(next ? 'auto_on' : 'auto_off');
    addLog(`User switched to ${next ? 'AUTO' : 'MANUAL'}`, 'warn');
  };

  const handleEstop = () => {
    if (window.confirm("TRIGGER EMERGENCY STOP?")) {
      sendUiCmd('stop');
      sendUiCmd('auto_off');
      addLog('USER TRIGGERED E-STOP', 'error');
    }
  };

  // --- Render ---

  return (
    <div className="min-h-screen bg-black text-gray-200 font-sans selection:bg-blue-500/30 overflow-hidden flex flex-col">
      
      {/* 1. TOP BAR: Status & criticals */}
      <header className="h-18 bg-gray-900 border-b border-gray-800 p-4 flex gap-4 items-center justify-between shrink-0 z-50">
        <div className="flex items-center gap-6">
          <div className="flex flex-col">
             <div className="flex items-center gap-2 text-blue-400">
               <Cpu className="animate-spin-slow" size={20} />
               <span className="font-bold text-lg tracking-wider">NIGHTFALL</span>
             </div>
             <div className="text-[10px] text-gray-500 font-mono tracking-widest pl-7">MISSION CONTROL</div>
          </div>
          
          <ConnectionBadge status={connectionStatus} stats={connectionStats} lastPing={lastPing} />
        </div>

        <div className="flex items-center gap-4">
           {/* FSM PILL */}
           <div className={`px-5 py-2 rounded-lg font-bold tracking-wider text-sm flex items-center gap-3 border shadow-lg transition-all ${
             isEmerg ? 'bg-red-600 border-red-400 text-white animate-pulse' : 
             isAuto ? 'bg-purple-600 border-purple-400 text-white' : 
             'bg-gray-800 border-gray-600 text-gray-300'
           }`}>
            <div>
              <div className="text-[10px] opacity-70 leading-none mb-1">SYSTEM STATE</div>
              <div className="flex items-center gap-2">
                 <Activity size={16} />
                 {state.nav_state === 'forward' && isAuto && <span className="animate-pulse">▶</span>}
                 {state.fsm || 'INIT'}
              </div>
            </div>
          </div>
          
          <button 
           onClick={handleEstop}
           className="bg-red-600 hover:bg-red-700 active:scale-95 text-white px-6 py-2 rounded-lg font-bold text-lg flex items-center gap-2 shadow-[0_0_15px_rgba(220,38,38,0.5)] border border-red-500 transition-all"
          >
            <Power size={24} /> STOP
          </button>
        </div>
      </header>

      {/* 2. MAIN GRID */}
      <main className="flex-1 overflow-hidden p-4 grid grid-cols-12 grid-rows-12 gap-4">
        
        {/* LEFT: VISION (6 cols, 7 rows) */}
        <div className="col-span-6 row-span-7 relative bg-black rounded-xl border border-gray-800 overflow-hidden group">
           <img 
             src="http://192.168.4.1:81/stream" 
             className="w-full h-full object-contain"
             onError={(e) => { e.target.style.display = 'none'; }}
           />
           <div className="absolute top-4 left-4 flex gap-2">
             <span className="bg-red-600 text-white px-2 py-0.5 text-xs font-bold rounded animate-pulse">LIVE</span>
             <span className={`px-2 py-0.5 text-xs font-bold rounded ${network.camera ? 'bg-green-600/80 text-white' : 'bg-red-600/80 text-white'}`}>
                CAM: {network.camera ? 'OK' : 'LOST'}
             </span>
           </div>
           <div className="absolute inset-0 border-2 border-white/10 rounded-xl pointer-events-none group-hover:border-white/20 transition-all" />
        </div>

        {/* RIGHT: TELEMETRY (6 cols, 7 rows) */}
        <div className="col-span-6 row-span-7 grid grid-rows-2 gap-4">
           {/* Top: Safety */}
           <div className="grid grid-cols-2 gap-4">
              <GasGauge value={sensors.gas} trend={history.gas} />
              <div className="flex flex-col gap-4">
                 <DistanceWidget front={sensors.front_dist} rear={sensors.rear_dist} trend={history.front} />
                 <div className="bg-gray-800/40 rounded-lg p-3 border border-gray-700 flex justify-between items-center">
                    <span className="text-gray-400 text-xs font-bold">FRONT ESP</span>
                    <span className={`text-xs font-mono px-2 py-1 rounded ${network.front ? 'bg-emerald-500/20 text-emerald-400' : 'bg-red-500/20 text-red-500'}`}>
                      {network.front ? 'ONLINE' : 'OFFLINE'}
                    </span>
                 </div>
              </div>
           </div>

           {/* Bot: Motor Telemetry */}
           <Card title="Drive Train" icon={Gauge} className="h-full">
              <div className="grid grid-cols-4 gap-4 h-full">
                 {[ 
                   { l: 'FL', v: motors.front_left }, { l: 'FR', v: motors.front_right },
                   { l: 'RL', v: motors.rear_left },  { l: 'RR', v: motors.rear_right } 
                 ].map((m, i) => (
                   <div key={i} className="bg-gray-950 rounded p-2 flex flex-col justify-end relative overflow-hidden border border-gray-800">
                      <div 
                         className="absolute bottom-0 left-0 right-0 bg-blue-900/30 transition-all duration-200" 
                         style={{ height: `${(m.v/255)*100}%` }} 
                      />
                      <div className="relative z-10 text-center">
                        <div className="text-[10px] text-gray-500 mb-1">{m.l}</div>
                        <div className="font-mono font-bold text-blue-300">{m.v}</div>
                      </div>
                   </div>
                 ))}
              </div>
           </Card>
        </div>

        {/* BOTTOM: LOGS (7 cols, 5 rows) */}
        <div className="col-span-7 row-span-5 relative">
          <Card title="Mission Log" icon={Terminal} className="h-full">
             <div className="flex-1 overflow-y-auto font-mono text-[11px] space-y-1 pr-2 max-h-[220px] scrollbar-thin scrollbar-thumb-gray-800">
               {logs.length === 0 && <div className="text-gray-600 italic p-4 text-center">System Ready. Waiting for events...</div>}
               {logs.map((l, i) => (
                 <div key={i} className="border-l-2 border-gray-800 pl-2 hover:bg-gray-800/20 py-0.5 rounded-r transition-colors">
                   <span className="text-gray-500 mr-3 opacity-50">{l.t}</span>
                   <span className={l.c}>{l.m}</span>
                 </div>
               ))}
               <div ref={logsEndRef} />
             </div>
          </Card>
        </div>

        {/* BOTTOM RIGHT: CONTROLS (5 cols, 5 rows) */}
        <div className="col-span-5 row-span-5 flex flex-col gap-4">
           {/* Auto Toggle */}
           <button 
             onClick={toggleAuto}
             disabled={isEmerg}
             className={`w-full py-4 rounded-xl font-bold flex items-center justify-center gap-3 transition-all ${
               isEmerg ? 'bg-gray-800 text-gray-500 cursor-not-allowed border border-gray-700' :
               isAuto ? 'bg-purple-600 hover:bg-purple-700 text-white shadow-lg shadow-purple-900/40 border-t border-purple-400' : 
               'bg-gray-800 hover:bg-gray-700 text-gray-200 border border-gray-600'
             }`}
           >
              {isAuto ? <Zap size={20} className="fill-current" /> : <Terminal size={20} />}
              <div className="flex flex-col items-start leading-none">
                <span className="text-xs opacity-60 font-medium tracking-wider">{isAuto ? 'OPERATOR' : 'ACTIVATE'}</span>
                <span className="text-lg">{isAuto ? 'AUTO MODE ACTIVE' : 'ENABLE AUTONOMY'}</span>
              </div>
           </button>

           {/* D-PAD */}
           <div className={`flex-1 bg-gray-900 rounded-xl border border-gray-800 p-4 flex items-center justify-center relative ${isAuto || isEmerg ? 'opacity-50 pointer-events-none grayscale' : ''}`}>
              { (isAuto || isEmerg) && (
                <div className="absolute inset-0 z-20 flex items-center justify-center bg-black/60 rounded-xl backdrop-blur-[1px]">
                  <div className="bg-gray-900 px-3 py-1 rounded border border-gray-700 text-xs font-mono text-gray-400 flex items-center gap-2">
                    <AlertCircle size={12} /> CONTROLS LOCKED
                  </div>
                </div>
              )}
              
              <div className="grid grid-cols-3 gap-2">
                 <div />
                 <button onMouseDown={() => handleControl('forward')} onMouseUp={() => handleControl('stop')} className="w-14 h-14 bg-gray-800 rounded-lg hover:bg-blue-600 active:bg-blue-500 transition-colors flex items-center justify-center"><ArrowUp/></button>
                 <div />
                 <button onMouseDown={() => handleControl('left')} onMouseUp={() => handleControl('stop')} className="w-14 h-14 bg-gray-800 rounded-lg hover:bg-blue-600 active:bg-blue-500 transition-colors flex items-center justify-center"><ArrowLeft/></button>
                 <button onMouseDown={() => handleControl('backward')} onMouseUp={() => handleControl('stop')} className="w-14 h-14 bg-gray-800 rounded-lg hover:bg-blue-600 active:bg-blue-500 transition-colors flex items-center justify-center"><ArrowDown/></button>
                 <button onMouseDown={() => handleControl('right')} onMouseUp={() => handleControl('stop')} className="w-14 h-14 bg-gray-800 rounded-lg hover:bg-blue-600 active:bg-blue-500 transition-colors flex items-center justify-center"><ArrowRight/></button>
              </div>
           </div>
        </div>

      </main>
    </div>
  );
}
