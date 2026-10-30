import { TrainScene } from '../features/playground/TrainScene';

export function PlaygroundPage() {
  return (
    <div className="min-h-screen bg-slate-950 p-8">
      <div className="max-w-6xl mx-auto">
        <div className="mb-8">
          <h1 className="text-4xl font-bold text-white mb-2">🚂 3D Playground</h1>
          <p className="text-slate-400">
            Тестовая страница для экспериментов с Three.js и React Three Fiber.
          </p>
        </div>

        {/* 3D Сцена */}
        <TrainScene />

        {/* Инструкция */}
        <div className="mt-6 p-4 bg-slate-800 rounded-lg">
          <h2 className="text-xl font-semibold text-white mb-2">🎮 Управление:</h2>
          <ul className="text-slate-300 space-y-1">
            <li>• <span className="font-mono bg-slate-700 px-2 py-0.5 rounded">ЛКМ</span> — вращение камеры</li>
            <li>• <span className="font-mono bg-slate-700 px-2 py-0.5 rounded">Колесо</span> — приближение/отдаление</li>
            <li>• <span className="font-mono bg-slate-700 px-2 py-0.5 rounded">ПКМ</span> — перемещение сцены</li>
          </ul>
        </div>
      </div>
    </div>
  );
}