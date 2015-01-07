package com.datasoft.ceres;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.widget.ProgressBar;

public class TextProgressBar extends ProgressBar {
	
	private String m_text = "";
	private int m_textColor = Color.WHITE;
	private float m_textSize = 16;
	private Paint tp = new Paint();
	private Rect bounds = new Rect();
	
	public TextProgressBar(Context ctx)
	{
		super(ctx);
	}
	
	public TextProgressBar(Context ctx, AttributeSet attrs)
	{
		super(ctx, attrs);
	}
	
	public TextProgressBar(Context ctx, AttributeSet attrs, int defStyle)
	{
		super(ctx, attrs, defStyle);
	}
	
	public String getText()
	{
		return m_text;
	}
	
	@Override
	protected synchronized void onDraw(Canvas canvas)
	{
		super.onDraw(canvas);
		tp.setAntiAlias(true);
		tp.setColor(m_textColor);
		float scale = getContext().getResources().getDisplayMetrics().density;
		float adjustedTextSize = ((m_textSize * scale) + 0.5f);
		tp.setTextSize(adjustedTextSize);
		tp.getTextBounds(m_text, 0, m_text.length(), bounds);
		int x = (getWidth() / 2) - bounds.centerX();
		int y = (getHeight() / 2) - bounds.centerY();
		canvas.drawText(m_text, x, y, tp);
	}
	
	public synchronized void setText(String text)
	{
		if(text != null)
		{
			m_text = text;
		}
		else
		{
			m_text = "";
		}
		postInvalidate();
	}
	
	public int getTextColor()
	{
		return m_textColor;
	}
	
	public synchronized void setTextColor(int tc)
	{
		m_textColor = tc;
		postInvalidate();
	}
	
	public float getTextSize()
	{
		return m_textSize;
	}
	
	public synchronized void setTextSize(float ts)
	{
		m_textSize = ts;
		postInvalidate();
	}
}
