import { Component, ElementRef, AfterViewInit, ViewChild, OnDestroy } from '@angular/core';
import { CommonModule } from '@angular/common';
import { RouterModule } from '@angular/router';
import { NavbarComponent } from '../../components/navbar/navbar';

@Component({
  selector: 'app-home',
  standalone: true,
  imports: [CommonModule, RouterModule, NavbarComponent],
  templateUrl: './home.html',
  styleUrl: './home.css'
})
export class HomeComponent implements AfterViewInit, OnDestroy {
  @ViewChild('waveCanvas') waveCanvasRef!: ElementRef<HTMLCanvasElement>;

  private animFrameId: number | null = null;
  private time = 0;

  features = [
    {
      icon: '🎵',
      title: 'Real-Time Pitch Detection',
      desc: 'FFT-powered analysis identifies notes as you play — with sub-100ms latency.'
    },
    {
      icon: '🎼',
      title: 'Live Sheet Music',
      desc: 'Notes appear on a virtual staff the moment they\'re detected, no manual input needed.'
    },
    {
      icon: '🧠',
      title: 'AI Noise Filtering',
      desc: 'A machine learning model strips background noise so only your playing counts.'
    },
    {
      icon: '📊',
      title: 'Practice History',
      desc: 'Every session is saved. Track your progress across weeks and songs.'
    },
  ];

  ngAfterViewInit(): void {
    this.startWaveAnimation();
  }

  ngOnDestroy(): void {
    if (this.animFrameId !== null) cancelAnimationFrame(this.animFrameId);
  }

  private startWaveAnimation(): void {
    const canvas = this.waveCanvasRef?.nativeElement;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const resize = () => {
      canvas.width = canvas.offsetWidth;
      canvas.height = canvas.offsetHeight;
    };
    resize();
    window.addEventListener('resize', resize);

    const draw = () => {
      this.time += 0.012;
      const { width, height } = canvas;
      ctx.clearRect(0, 0, width, height);

      const waves = [
        { amp: 28, freq: 0.018, speed: 1.0,  color: 'rgba(124,58,237,0.55)', lineW: 2.5 },
        { amp: 18, freq: 0.025, speed: 1.4,  color: 'rgba(168,85,247,0.40)', lineW: 1.5 },
        { amp: 10, freq: 0.040, speed: 0.8,  color: 'rgba(192,132,252,0.28)', lineW: 1.0 },
      ];

      waves.forEach(w => {
        ctx.beginPath();
        ctx.strokeStyle = w.color;
        ctx.lineWidth = w.lineW;
        ctx.lineJoin = 'round';

        for (let x = 0; x <= width; x += 2) {
          const y = height / 2
            + Math.sin(x * w.freq + this.time * w.speed) * w.amp
            + Math.sin(x * w.freq * 0.5 + this.time * w.speed * 0.6) * (w.amp * 0.4);
          x === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        }
        ctx.stroke();
      });

      this.animFrameId = requestAnimationFrame(draw);
    };

    draw();
  }
}