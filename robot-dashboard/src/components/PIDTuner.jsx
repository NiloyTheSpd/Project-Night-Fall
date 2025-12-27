import { useState } from 'react';

export default function PIDTuner({ sendUiCmd, telemetry, isConnected }) {
  const [kP, setKP] = useState(4.0);
  const [kI, setKI] = useState(0.0);
  const [kD, setKD] = useState(1.0);
  const [enabled, setEnabled] = useState(true);
  const [expanded, setExpanded] = useState(false);

  // Presets
  const presets = {
    conservative: { kP: 2.0, kI: 0.0, kD: 0.5 },
    balanced: { kP: 4.0, kI: 0.0, kD: 1.0 },
    aggressive: { kP: 8.0, kI: 0.1, kD: 2.0 }
  };

  const applyPID = () => {
    if (!isConnected) return;
    sendUiCmd('pid_tune', { kP, kI, kD });
  };

  const togglePID = () => {
    if (!isConnected) return;
    const newEnabled = !enabled;
    sendUiCmd('pid_enable', { enable: newEnabled });
    setEnabled(newEnabled);
  };

  const loadPreset = (preset) => {
    setKP(preset.kP);
    setKI(preset.kI);
    setKD(preset.kD);
    applyPID();
  };

  const control = telemetry?.control || { out: 0, err: 0, sp: 0, P: 0, I: 0, D: 0 };
  const timing = telemetry?.timing || { loop_us: 0 };

  return (
    <div className="bg-gray-900 rounded-lg p-4 border border-gray-700">
      <div 
        className="flex justify-between items-center mb-3 cursor-pointer" 
        onClick={() => setExpanded(!expanded)}
      >
        <div className="flex items-center gap-2">
          <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} 
                  d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" />
          </svg>
          <h3 className="font-bold text-white">PID Tuner</h3>
          {!enabled && (
            <span className="text-xs bg-red-600 px-2 py-0.5 rounded text-white">DISABLED</span>
          )}
          <span className="text-xs text-gray-400">({timing.loop_us}μs)</span>
        </div>
        <span className="text-gray-400">{expanded ? '▼' : '▶'}</span>
      </div>

      {expanded && (
        <>
          {/* Presets */}
          <div className="flex gap-2 mb-4">
            {Object.entries(presets).map(([name, preset]) => (
              <button 
                key={name} 
                onClick={() => loadPreset(preset)}
                disabled={!isConnected}
                className="flex-1 bg-gray-800 hover:bg-gray-700 disabled:bg-gray-800 disabled:opacity-50 disabled:cursor-not-allowed px-3 py-1 rounded text-sm text-white capitalize transition-colors"
              >
                {name}
              </button>
            ))}
          </div>

          {/* Sliders */}
          <div className="space-y-3 mb-4">
            <div>
              <div className="flex justify-between text-sm mb-1">
                <label className="text-gray-300">Proportional (kP)</label>
                <span className="text-white font-mono">{kP.toFixed(1)}</span>
              </div>
              <input 
                type="range" 
                min="0" 
                max="20" 
                step="0.5" 
                value={kP} 
                onChange={e => setKP(parseFloat(e.target.value))}
                disabled={!isConnected}
                className="w-full h-2 bg-gray-700 rounded-lg appearance-none cursor-pointer accent-blue-600 disabled:opacity-50 disabled:cursor-not-allowed"
              />
            </div>
            
            <div>
              <div className="flex justify-between text-sm mb-1">
                <label className="text-gray-300">Integral (kI)</label>
                <span className="text-white font-mono">{kI.toFixed(2)}</span>
              </div>
              <input 
                type="range" 
                min="0" 
                max="2" 
                step="0.05" 
                value={kI}
                onChange={e => setKI(parseFloat(e.target.value))}
                disabled={!isConnected}
                className="w-full h-2 bg-gray-700 rounded-lg appearance-none cursor-pointer accent-blue-600 disabled:opacity-50 disabled:cursor-not-allowed"
              />
            </div>
            
            <div>
              <div className="flex justify-between text-sm mb-1">
                <label className="text-gray-300">Derivative (kD)</label>
                <span className="text-white font-mono">{kD.toFixed(1)}</span>
              </div>
              <input 
                type="range" 
                min="0" 
                max="10" 
                step="0.5" 
                value={kD}
                onChange={e => setKD(parseFloat(e.target.value))}
                disabled={!isConnected}
                className="w-full h-2 bg-gray-700 rounded-lg appearance-none cursor-pointer accent-blue-600 disabled:opacity-50 disabled:cursor-not-allowed"
              />
            </div>
          </div>

          {/* Live Readout */}
          <div className="bg-gray-800 p-3 rounded mb-3 border border-gray-700">
            <div className="text-xs text-gray-400 mb-2">Live Control Data</div>
            <div className="grid grid-cols-3 gap-2 text-xs font-mono">
              <div className="bg-gray-900 p-2 rounded">
                <div className="text-gray-400">Output</div>
                <div className="text-green-400 font-bold">{control.out?.toFixed(1) || '—'}</div>
              </div>
              <div className="bg-gray-900 p-2 rounded">
                <div className="text-gray-400">Error</div>
                <div className="text-yellow-400 font-bold">{control.err?.toFixed(1) || '—'}</div>
              </div>
              <div className="bg-gray-900 p-2 rounded">
                <div className="text-gray-400">Setpoint</div>
                <div className="text-blue-400 font-bold">{control.sp?.toFixed(1) || '—'}</div>
              </div>
              <div className="bg-gray-900 p-2 rounded">
                <div className="text-gray-400">P</div>
                <div className="text-white">{control.P?.toFixed(1) || '—'}</div>
              </div>
              <div className="bg-gray-900 p-2 rounded">
                <div className="text-gray-400">I</div>
                <div className="text-white">{control.I?.toFixed(1) || '—'}</div>
              </div>
              <div className="bg-gray-900 p-2 rounded">
                <div className="text-gray-400">D</div>
                <div className="text-white">{control.D?.toFixed(1) || '—'}</div>
              </div>
            </div>
          </div>

          {/* Actions */}
          <div className="flex gap-2">
            <button 
              onClick={applyPID} 
              disabled={!isConnected}
              className="flex-1 bg-blue-600 hover:bg-blue-700 disabled:bg-gray-700 disabled:cursor-not-allowed px-4 py-2 rounded flex items-center justify-center gap-2 text-white font-medium transition-colors"
            >
              <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} 
                      d="M8 7H5a2 2 0 00-2 2v9a2 2 0 002 2h14a2 2 0 002-2V9a2 2 0 00-2-2h-3m-1 4l-3 3m0 0l-3-3m3 3V4" />
              </svg>
              Apply
            </button>
            <button 
              onClick={togglePID} 
              disabled={!isConnected}
              className={`flex-1 ${enabled ? 'bg-yellow-600 hover:bg-yellow-700' : 'bg-green-600 hover:bg-green-700'} disabled:bg-gray-700 disabled:cursor-not-allowed px-4 py-2 rounded flex items-center justify-center gap-2 text-white font-medium transition-colors`}
            >
              <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} 
                      d="M13 10V3L4 14h7v7l9-11h-7z" />
              </svg>
              {enabled ? 'Disable' : 'Enable'}
            </button>
          </div>
        </>
      )}
    </div>
  );
}
